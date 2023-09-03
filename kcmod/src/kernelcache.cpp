// Copyright (c) skr0x1c0 2023.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <fstream>
#include <filesystem>
#include <set>

#include <CoreFoundation/CFNumber.h>
#include <Kernel/mach/kmod.h>

#include <nlohmann/json.hpp>

#include "common.h"
#include "aarch64.h"
#include "core_foundation.h"
#include "fixup_chain.h"
#include "hooks.h"
#include "kernelcache.h"
#include "log.h"
#include "macho.h"
#include "split_seg.h"
#include "symidx.h"


using namespace kcmod;

namespace fs = std::filesystem;
using json = nlohmann::json;


void KernelCache::replace_fileset(const std::string& fileset, const KernelExtension &kext, const std::optional<fs::path>& symbols) {
    // Verify kext segments
    const std::set<std::string> k_expected_segments = {
        "__TEXT", "__TEXT_EXEC", "__DATA_CONST", "__DATA", "__LINKEDIT"
    };
    for (const auto& segment: kext.read_segments()) {
        if (k_expected_segments.find(segment->segname) == k_expected_segments.end()) {
            throw FatalError("Unexpected segment: {}", std::string(segment->segname));
        }
    }

    // Remove dyld chained fixups in victim fileset
    {
        DyldFixupChainEditor fixup_editor {MachOBinary<char>{data_}};
        for (const auto* segment: read_fs_segments(fileset)) {
            std::string segname {segment->segname};
            if (segname == "__DATA_CONST" || segname == "__DATA") {
                fixup_editor.remove_fixups(segment->fileoff, segment->filesize);
            }
        }
    }

    // Copy segments from kext to victim fileset
    if (kext.read_segment("__TEXT_EXEC")) {
        replace_segment(fileset, kext, "__TEXT_EXEC");
    }
    if (kext.read_segment("__DATA_CONST")) {
        replace_segment(fileset, kext, "__DATA_CONST");
    }
    if (kext.read_segment("__DATA")) {
        replace_segment(fileset, kext, "__DATA");
    }
    if (kext.read_segment("__TEXT")) {
        replace_text_segment(fileset, kext);
    } else {
        throw FatalError("Kext missing __TEXT segment");
    }

    // Apply split segment info
    apply_split_segment_fixups(fileset, kext);

    // Remove victim fileset prelink info
    remove_prelink_info(fileset);

    // Replace fileset id
    replace_fileset_id(fileset, kext.bundle_id());

    // Insert kext prelink info
    insert_kext_prelink_info(kext);

    // setup kmod info
    {
        std::map<std::string, const segment_command_64*> fileset_segments;
        for (const auto* segment: read_fs_segments(kext.bundle_id())) {
            fileset_segments[segment->segname] = segment;
        }
        SpanReader reader{data_, fileset_segments["__DATA"]->fileoff};
        auto* info = reader.read<kmod_info>();
        info->address = fileset_segments["__TEXT"]->vmaddr;
        info->size = fileset_segments["__TEXT"]->vmsize;
    }

    // Link kext
    bind_kext_symbols(kext, symbols);

    // Setup hooks
    bind_hooks(kext, symbols);
}

void KernelCache::bind_hooks(const KernelExtension &kext, const std::optional<std::filesystem::path>& symbols_json) {
    KCModHookReader reader {kext.binary_data(), 0};
    SymbolRegistry registry = construct_symbol_registry(kext, symbols_json);

    std::map<std::string, const segment_command_64*> kext_segments;
    for (const auto& segment: kext.read_segments()) {
        kext_segments[segment->segname] = segment;
    }

    std::map<std::string, const segment_command_64*> fileset_segments;
    for (const auto& segment: read_fs_segments(kext.bundle_id())) {
        fileset_segments[segment->segname] = segment;
    }

    const auto* kext_text_exec = kext_segments["__TEXT_EXEC"];
    const auto* fileset_text_exec = fileset_segments["__TEXT_EXEC"];

    for (const auto& hook: reader.read_hooks()) {
        Symbol fn_symbol = registry.find_bind_symbol(hook.fn_name);
        MachOBinary kc_binary {data_, 0};
        const auto* fn_segment = kc_binary.find_segment_with_va(fn_symbol.vmaddr);
        kcmod_decode_verify(fn_segment != nullptr);
        uint64_t fn_segment_offset = fn_symbol.vmaddr - fn_segment->vmaddr;
        uint64_t hook_fn_segment_offset = hook.hook_fn.vmaddr - kext_text_exec->vmaddr;
        uint64_t super_fn_segment_offset = hook.super_fn.vmaddr - kext_text_exec->vmaddr;
        uint64_t hook_fn_kc_vmaddr = fileset_text_exec->vmaddr + hook_fn_segment_offset;
        uint64_t super_fn_kc_vmaddr = fileset_text_exec->vmaddr + super_fn_segment_offset;

        int64_t fn_offset = hook_fn_kc_vmaddr - fn_symbol.vmaddr;
        SpanWriter fn_writer{data_, fn_segment->fileoff + fn_segment_offset};
        if (aarch64::is_bti_instr(*fn_writer.peek<uint32_t>())) {
            fn_writer.seek(4);
            fn_offset -= 4;
        }

        uint32_t start_instr = *fn_writer.peek<uint32_t>();
        fn_writer.write(aarch64::Branch{fn_offset, false}.encode());

        SpanWriter super_fn_writer {data_, fileset_text_exec->fileoff + super_fn_segment_offset};
        if (aarch64::is_bti_instr(*super_fn_writer.peek<uint32_t>())) {
            super_fn_writer.seek(4);
            super_fn_kc_vmaddr += 4;
        }

        for (uint32_t instr: aarch64::build_hook_super_fn(
                 start_instr, fn_symbol.vmaddr, super_fn_kc_vmaddr)) {
            kcmod_verify(*super_fn_writer.peek<uint32_t>() == 0xd4200020); // brk instruction
            super_fn_writer.write(instr);
        }
    }
}

std::vector<std::string> KernelCache::read_kext_deps(const KernelExtension &kext) {
    PropertyList plist = kext.read_info_plist();
    cf::Dictionary dict {static_cast<CFDictionaryRef>(plist.plist())};
    std::vector<std::string> entries = dict.read_dict("OSBundleLibraries").keys();
    std::set<std::string> deps;
    for (const auto& entry: entries) {
        if (entry.starts_with("com.apple.kpi.")) {
            deps.insert("com.apple.kernel");
        } else {
            deps.insert(entry);
        }
    }
    return std::vector<std::string>{deps.begin(), deps.end()};
}

SymbolRegistry KernelCache::construct_symbol_registry(const KernelExtension& kext, const std::optional<std::filesystem::path>& symbols_json) {
    SymbolRegistry registry;
    std::vector<std::string> kext_deps = read_kext_deps(kext);
    for (const auto& fileset_id: kext_deps) {
        const auto* fileset = read_fileset(fileset_id);
        if (!fileset) {
            throw FatalError {
                "Dependency {} for kext {} not present in kernelcache",
                fileset_id,
                kext.bundle_id()
            };
        }
        registry.index_binary(data_, fileset->fileoff);
    }
    if (symbols_json) {
        kcmod_verify(std::filesystem::exists(*symbols_json));
        std::ifstream json_file {symbols_json->string()};
        json data = json::parse(json_file);
        std::set<std::string> kext_deps_set {kext_deps.begin(), kext_deps.end()};
        for (const auto& [fileset, v]: data.get<json::object_t>()) {
            if (!kext_deps_set.contains(fileset)) {
                continue;
            }
            for (const auto& [symbol, addr]: v.get<json::object_t>()) {
                uint64_t vmaddr = 0;
                if (addr.is_number()) {
                    vmaddr = addr.get<uint64_t>();
                } else if (addr.is_string()) {
                    vmaddr = std::stoull(addr.get<std::string>(), nullptr, 16);
                } else {
                    throw FatalError {"Invalid symbols json entry at {}::{}", fileset, symbol};
                }
                // TODO: add duplicate override debug message
                registry.add_override(symbol, vmaddr);
            }
        }
    }
    return registry;
}

void KernelCache::bind_kext_symbols(const KernelExtension &kext, const std::optional<std::filesystem::path>& symbols_json) {
    std::map<std::string, segment_command_64*> fileset_segments;
    for (const auto* segment: read_fs_segments(kext.bundle_id())) {
        fileset_segments[segment->segname] = const_cast<segment_command_64*>(segment);
    }
    uint64_t vm_base = MachOBinary{data_}.vm_base();
    SymbolRegistry registry = construct_symbol_registry(kext, symbols_json);
    DyldFixupChainEditor kc_dyld_editor{MachOBinary{data_}};
    DyldFixupChainEditor kext_dyld_reader {MachOBinary{
        // TODO: cleanup
        std::span<char>{(char*)kext.binary_data().data(), kext.binary_data().size()}
    }};
    MachOBinary kext_binary = kext.binary();
    std::vector<DyldChainedImport> imports = kext_dyld_reader.read_chained_imports();
    std::vector<DyldFixupPointer*> fixups = kext_dyld_reader.read_fixups();
    for (const auto& v: fixups) {
        if (!v->bind) {
            // TODO: verify target is in kc??
            continue;
        }

        uint64_t ptr_kext_fileoff = kext_binary.fileoff_from_pointer(
            reinterpret_cast<const char*>(v));
        const segment_command_64* kext_segment = kext_binary.find_segment_with_fileoff(ptr_kext_fileoff);
        const segment_command_64* fileset_segment = fileset_segments[kext_segment->segname];
        kcmod_verify(ptr_kext_fileoff >= kext_segment->fileoff);
        uint64_t ptr_segment_offset = ptr_kext_fileoff - kext_segment->fileoff;
        SpanReader kc_reader {data_, fileset_segment->fileoff};
        kc_reader.seek(ptr_segment_offset);
        DyldFixupPointer* kc_ptr = kc_reader.peek<DyldFixupPointer>();
        kcmod_verify(kc_ptr->raw == v->raw);

        uint64_t ordinal = v->auth ? v->ptr_auth_bind.ordinal : v->ptr_bind.ordinal;
        uint64_t addend = v->auth ? 0 : v->ptr_bind.addend;
        kcmod_verify(addend == 0);
        std::vector<Symbol> symbols;
        kcmod_decode_verify(ordinal < imports.size());
        std::string name = imports[ordinal].symbol_name;
        const auto symbol = registry.find_bind_symbol(name);
        uint64_t target = symbol.vmaddr;
        kcmod_verify(target >= vm_base);
        uint64_t offset = target - vm_base;

        DyldFixupPointer kc_fixup;
        if (v->auth) {
            dyld_chained_ptr_arm64e_auth_rebase result;
            result.target = offset;
            result.auth = 1;
            result.bind = 0;
            result.key = v->ptr_auth_bind.key;
            result.diversity = v->ptr_auth_bind.diversity;
            result.addrDiv = v->ptr_auth_bind.addrDiv;
            result.next = 0;
            kc_fixup.ptr_auth_rebase = result;
        } else {
            dyld_chained_ptr_arm64e_rebase result;
            result.target = offset;
            result.auth = 0;
            result.bind = 0;
            result.high8 = 0;
            result.next = 0;
            kc_fixup.ptr_rebase = result;
        }
        kc_dyld_editor.add_fixup(kc_reader.cursor(),kc_fixup);
    }
}

void KernelCache::insert_kext_prelink_info(const KernelExtension &kext) {
    std::map<std::string, const segment_command_64*> fileset_segments;
    for (const auto* segment: read_fs_segments(kext.bundle_id())) {
        fileset_segments[segment->segname] = segment;
    }

    PropertyList plist = kext.read_info_plist();
    CFMutableDictionaryRef info_dict = const_cast<CFMutableDictionaryRef>(
        static_cast<CFDictionaryRef>(plist.plist()));
    // OSBundleUUID
    {
        const auto* kext_uuid = kext.binary().read_uuid();
        CFDataRef cf_kext_uuid = CFDataCreate(kCFAllocatorDefault, kext_uuid->uuid, sizeof(kext_uuid->uuid));
        CFDictionarySetValue(
            info_dict,
            CFSTR("OSBundleUUID"),
            cf_kext_uuid);
        CFRelease(cf_kext_uuid);
    }
    // _InfoPlistDigest??

    // _PrelinkBundlePath
    {
        std::string kext_name = fs::path{"/System/Library/Extensions"} / kext.path().filename();
        CFStringRef cf_kext_name = CFStringCreateWithCString(kCFAllocatorDefault, kext_name.c_str(), kCFStringEncodingUTF8);
        CFDictionarySetValue(
            info_dict,
            CFSTR("_PrelinkBundlePath"),
            cf_kext_name);
        CFRelease(cf_kext_name);
    }

    // _PrelinkExecutableLoadAddr
    // _PrelinkExecutableSourceAddr
    {
        CFNumberRef cf_load_addr = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &fileset_segments["__TEXT"]->vmaddr);
        CFDictionarySetValue(
            info_dict,
            CFSTR("_PrelinkExecutableLoadAddr"),
            cf_load_addr
        );
        CFDictionarySetValue(
            info_dict,
            CFSTR("_PrelinkExecutableSourceAddr"),
            cf_load_addr
        );
        CFRelease(cf_load_addr);
    }

    // _PrelinkExecutableRelativePath
    {
        std::string executable_path = relative(kext.binary_path(), kext.path());
        CFStringRef cf_relative_path = CFStringCreateWithCString(kCFAllocatorDefault, executable_path.c_str(), kCFStringEncodingUTF8);
        CFDictionarySetValue(
            info_dict,
            CFSTR("_PrelinkExecutableRelativePath"),
            cf_relative_path
        );
        CFRelease(cf_relative_path);
    }

    // _PrelinkExecutableSize
    {
        uint64_t size = fileset_segments["__TEXT"]->filesize;
        CFNumberRef cf_exec_size = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &size);
        CFDictionarySetValue(
            info_dict,
            CFSTR("_PrelinkExecutableSize"),
            cf_exec_size
        );
        CFRelease(cf_exec_size);
    }

    // _PrelinkKmodInfo
    {
        CFNumberRef cf_addr = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &fileset_segments["__DATA"]->vmaddr);
        CFDictionarySetValue(
            info_dict,
            CFSTR("_PrelinkKmodInfo"),
            cf_addr);
        CFRelease(cf_addr);
    }
    insert_prelink_info(info_dict);
}

void KernelCache::remove_prelink_info(const std::string &fileset) {
    PropertyList plist = read_prelink_info();
    CFMutableDictionaryRef plist_dict = const_cast<CFMutableDictionaryRef>(static_cast<CFDictionaryRef>(plist.plist()));
    CFArrayRef info_dicts = static_cast<CFArrayRef>(CFDictionaryGetValue(plist_dict, CFSTR("_PrelinkInfoDictionary")));
    for (size_t i=0; i < CFArrayGetCount(info_dicts); ++i) {
        CFDictionaryRef current_info_dict = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(info_dicts, i));
        CFStringRef cf_bundle_id = static_cast<CFStringRef>(CFDictionaryGetValue(current_info_dict, CFSTR("CFBundleIdentifier")));
        kcmod_verify(cf_bundle_id != nullptr);
        std::string bundle_id{CFStringGetCStringPtr(cf_bundle_id, kCFStringEncodingUTF8)};
        if (bundle_id == fileset) {
            CFArrayRemoveValueAtIndex(const_cast<CFMutableArrayRef>(info_dicts), i);
            write_prelink_info(plist_dict);
            return;
        }
    }
    throw FatalError{"Fileset {} not found in prelink info", fileset};
}

void KernelCache::insert_prelink_info(CFDictionaryRef info) {
    PropertyList plist = read_prelink_info();
    CFMutableDictionaryRef plist_dict = const_cast<CFMutableDictionaryRef>(static_cast<CFDictionaryRef>(plist.plist()));
    CFArrayRef info_dicts = static_cast<CFArrayRef>(CFDictionaryGetValue(plist_dict, CFSTR("_PrelinkInfoDictionary")));
    CFArrayAppendValue(const_cast<CFMutableArrayRef>(info_dicts), info);
    write_prelink_info(plist_dict);
}

void KernelCache::write_prelink_info(CFPropertyListRef plist) {
    const auto* segment = read_prelink_info_segment();
    SpanReader reader{data_, segment->fileoff};
    CFDataRef data = CFPropertyListCreateXMLData(kCFAllocatorDefault, plist);
    kcmod_verify(data != nullptr);
    kcmod_verify(0 < CFDataGetLength(data) <= segment->filesize);
    SpanWriter writer{reader.read_data(segment->filesize), 0};
    writer.put_zero(segment->filesize);
    writer.write(std::span<const char>{(const char*)CFDataGetBytePtr(data), (size_t)CFDataGetLength(data)});
    CFRelease(data);
}

PropertyList KernelCache::read_prelink_info() {
    const auto* segment = read_prelink_info_segment();
    SpanReader reader{data_, segment->fileoff};
    return PropertyList{reader.read_data(segment->filesize)};
}

void KernelCache::apply_split_segment_fixups(const std::string &fileset, const KernelExtension &kext) {
    uint64_t kc_vm_base = MachOBinary{data_, 0}.vm_base();
    std::vector<DyldCacheAdjV2Entry> entries = parse_split_seg_info(kext.binary_data());
    std::vector<const section_64*> kext_sections;
    std::vector<const section_64*> fileset_sections;
    for (const auto* segment: kext.read_segments()) {
        for (const auto* section: kext.read_sections(segment->segname)) {
            kext_sections.push_back(section);
        }
        for (const auto* section: read_fs_sections(fileset, segment->segname)) {
            fileset_sections.push_back(section);
        }
    }
    kcmod_verify(kext_sections.size() == fileset_sections.size());
    for (const auto& entry: entries) {
        kcmod_decode_verify(entry.from_section_idx >= 1);
        kcmod_decode_verify(entry.from_section_idx <= kext_sections.size());
        kcmod_decode_verify(entry.to_section_idx >= 1);
        kcmod_decode_verify(entry.to_section_idx <= kext_sections.size());
        const auto* kext_from_section = kext_sections[entry.from_section_idx - 1];
        const auto* kext_to_section = kext_sections[entry.to_section_idx - 1];
        const auto* fileset_from_section = fileset_sections[entry.from_section_idx - 1];
        const auto* fileset_to_section = fileset_sections[entry.to_section_idx - 1];
        std::string from_segment_name {kext_from_section->segname};
        std::string to_segment_name {kext_to_section->segname};
        switch (entry.kind) {
            case DyldCacheAdjV2Kind::Arm64Adrp: {
                SpanReader instr_reader {data_, fileset_from_section->offset};
                instr_reader.seek(entry.from_section_offset);
                uint64_t to_addr = (fileset_to_section->addr + entry.to_section_offset) & ~4095ULL;
                uint64_t from_addr = (fileset_from_section->addr + entry.from_section_offset) & ~4095ULL;
                auto* instr_raw = instr_reader.read<uint32_t>();
                aarch64::Adrp instr{*instr_raw};
                instr.set_imm(to_addr - from_addr);
                *instr_raw = instr.encode();
                break;
            }
            case DyldCacheAdjV2Kind::Arm64Off12: {
                SpanReader instr_reader {data_, fileset_from_section->offset};
                instr_reader.seek(entry.from_section_offset);
                uint64_t to_addr = fileset_to_section->addr + entry.to_section_offset;
                uint64_t offset = to_addr & 0xfffLL;
                auto* instr_raw = instr_reader.read<uint32_t>();
                
                if (auto instr = aarch64::parse_instr<aarch64::AddImm>(*instr_raw)) {
                    instr->set_imm(offset);
                    *instr_raw = instr->encode();
                    break;
                }

                if (auto instr = aarch64::parse_instr<aarch64::LdrImmediate>(*instr_raw)) {
                    instr->set_imm(offset);
                    *instr_raw = instr->encode();
                    break;
                }

                kcmod_todo();
            }
            case DyldCacheAdjV2Kind::Pointer64:
            case DyldCacheAdjV2Kind::ThreadedPointer64: {
                SpanReader reader {data_, fileset_from_section->offset};
                reader.seek(entry.from_section_offset);
                auto dyld_ptr = *reader.peek<DyldFixupPointer>();
                kcmod_verify(!dyld_ptr.bind);
                uint64_t target = fileset_to_section->addr + entry.to_section_offset;
                if (dyld_ptr.auth) {
                    dyld_ptr.ptr_auth_rebase.target = target - kc_vm_base;
                } else {
                    dyld_ptr.ptr_rebase.target = target - kc_vm_base;
                }
                dyld_ptr.next = 0;
                DyldFixupChainEditor editor{MachOBinary{data_, 0}};
                editor.add_fixup(reader.cursor(), dyld_ptr);
                break;
            }
            case DyldCacheAdjV2Kind::Arm64Br26:
            case DyldCacheAdjV2Kind::ArmBr24: {
                if (from_segment_name == to_segment_name) {
                    // TODO: optimizations
                    break;
                }
                kcmod_todo();
            }
            case DyldCacheAdjV2Kind::Pointer32:
            case DyldCacheAdjV2Kind::Delta32:
            case DyldCacheAdjV2Kind::Delta64:
            case DyldCacheAdjV2Kind::ArmMovwMovt:
            case DyldCacheAdjV2Kind::ThumbMovwMovt:
            case DyldCacheAdjV2Kind::ThumbBr22:
            case DyldCacheAdjV2Kind::ImageOff32:
                kcmod_todo();
            case DyldCacheAdjV2Kind::Invalid:
                kcmod_not_reachable();
        }
    }
}

void KernelCache::replace_fileset_id(const std::string &from, const std::string &to) {
    const auto *cmd = read_fileset(from);
    size_t max_id_len = cmd->cmdsize - cmd->entry_id.offset - 1;
    if (to.size() > max_id_len) {
        throw FatalError{"replacement kext id {} is too long", to};
    }
    char *id = (char *) cmd + cmd->entry_id.offset;
    SpanWriter writer{data_, static_cast<uint64_t>(id - data_.data())};
    writer.write(std::span{to.data(), to.size()});
    writer.write_zero(max_id_len - to.size() + 1);
}

void KernelCache::replace_segment(const std::string &fileset_id, const KernelExtension &kext,
                                  const std::string &segment_name) {
    const auto* victim_segment = read_fs_segment(fileset_id, segment_name);
    kcmod_verify(victim_segment->filesize <= victim_segment->vmsize);
    uint64_t available_size = victim_segment->filesize;
    const auto* replacement_segment = kext.read_segment(segment_name);
    size_t replacement_data_size = kext.binary().read_used_segment_size(segment_name);
    if (replacement_data_size > available_size) {
        throw FatalError{"replacement kext segment {} is too large", segment_name};
    }
    SpanWriter writer{data_, victim_segment->fileoff};
    SpanReader reader{kext.binary_data(), replacement_segment->fileoff};
    writer.write(reader.read_data(replacement_data_size));
    writer.write_zero(available_size - replacement_data_size);
}

void KernelCache::replace_text_segment(const std::string &fileset_id, const KernelExtension &kext) {
    const auto* fileset = read_fileset(fileset_id);

    std::map<std::string, segment_command_64> current_segments;
    for (const auto& segment : read_fs_segments(fileset_id)) {
        current_segments[segment->segname] = *segment;
    }

    const segment_command_64* src_text_cmd = kext.read_segment("__TEXT");
    kcmod_verify(src_text_cmd->fileoff == 0);
    const segment_command_64* dst_text_cmd = &current_segments["__TEXT"];
    kcmod_verify(dst_text_cmd->fileoff == fileset->fileoff);

    kcmod_verify(kext.binary().read_used_segment_size("__TEXT") <= dst_text_cmd->filesize);

    // Copy commands to kernel cache
    {
        SpanWriter writer{data_, dst_text_cmd->fileoff};
        SpanReader reader{kext.binary_data(), src_text_cmd->fileoff};

        const mach_header_64* header = reader.read<mach_header_64>();
        writer.write(*header);

        size_t ncmds = 0;
        size_t sizeofcmds = 0;
        for (size_t i = 0; i < header->ncmds; ++i) {
            const load_command* cmd = reader.peek<load_command>();
            switch (cmd->cmd) {
                case LC_SEGMENT_64:
                case LC_UUID:
                case LC_SYMTAB:
                case LC_BUILD_VERSION:
                case LC_DYSYMTAB: {
                    writer.write(reader.peek_data(cmd->cmdsize));
                    ncmds++;
                    sizeofcmds += cmd->cmdsize;
                    break;
                }
                case LC_DYLD_CHAINED_FIXUPS:
                case LC_SEGMENT_SPLIT_INFO:
                    break;
            }
            reader.seek(cmd->cmdsize);
        }

        SpanReader text_reader{data_, dst_text_cmd->fileoff};
        mach_header_64* new_header = text_reader.read<mach_header_64>();
        new_header->ncmds = ncmds;
        new_header->sizeofcmds = sizeofcmds;

        size_t wrote_bytes = writer.cursor() - dst_text_cmd->fileoff;
        kcmod_verify(wrote_bytes < dst_text_cmd->filesize);
        writer.write_zero(dst_text_cmd->filesize - wrote_bytes);
    }

    // Fixup header
    {
        SpanReader reader {data_, dst_text_cmd->fileoff};
        auto* header = reader.read<mach_header_64>();
        header->flags |= MH_DYLIB_IN_CACHE;
    }

    // Fixup commands
    {
        SpanReader reader{data_, dst_text_cmd->fileoff};
        const mach_header_64* header = reader.read<mach_header_64>();
        for (size_t i=0; i<header->ncmds; ++i) {
            load_command* cmd = reader.peek<load_command>();
            switch (cmd->cmd) {
                case LC_SEGMENT_64: {
                    SpanReader seg_reader{data_, reader.cursor()};
                    segment_command_64* new_segment = seg_reader.read<segment_command_64>();
                    const auto &old_segment = current_segments[new_segment->segname];
                    int64_t vmaddr_delta = old_segment.vmaddr - new_segment->vmaddr;
                    new_segment->vmaddr = old_segment.vmaddr;
                    new_segment->vmsize = old_segment.vmsize;
                    int64_t fileoff_delta = old_segment.fileoff - new_segment->fileoff;
                    new_segment->fileoff = old_segment.fileoff;
                    new_segment->filesize = old_segment.filesize;
                    for (size_t sect_idx = 0; sect_idx < new_segment->nsects; ++sect_idx) {
                        auto* section = seg_reader.read<section_64>();
                        section->addr += vmaddr_delta;
                        section->offset += fileoff_delta;
                        kcmod_verify(section->reloff == 0);
                    }
                    break;
                }
                case LC_SYMTAB: {
                    symtab_command* symtab = reader.peek<symtab_command>();
                    symtab->nsyms = 0;
                    symtab->strsize = 0;
                    break;
                }
                case LC_DYSYMTAB:
                case LC_BUILD_VERSION:
                case LC_UUID: {
                    break;
                }
                default: {
                    throw FatalError{"Unexpected load command: {}", cmd->cmd};
                }
            }
            reader.seek(cmd->cmdsize);
        }
    }

    // Copy sections
    for (const auto* section: kext.read_sections("__TEXT")) {
        std::span<char> text_section = data_.subspan(dst_text_cmd->fileoff, dst_text_cmd->filesize);
        SpanWriter writer{text_section, section->offset - src_text_cmd->fileoff};
        SpanReader reader{kext.binary_data(), section->offset};
        writer.write(reader.read_data(section->size));
    }
}

fileset_entry_command * KernelCache::read_fileset(const std::string &fileset) {
    MachOBinary binary {data_};
    auto filesets = binary.read_filesets();
    if (auto it = filesets.find(fileset); it != filesets.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<segment_command_64 *> KernelCache::read_fs_segments(const std::string &fileset_name) {
    auto* fileset = read_fileset(fileset_name);
    MachOBinary binary {data_, fileset->fileoff};
    return binary.read_segments();
}

segment_command_64 *KernelCache::read_fs_segment(const std::string &fileset_name, const std::string &segment_name) {
    auto* fileset = read_fileset(fileset_name);
    MachOBinary binary {data_, fileset->fileoff};
    return binary.read_segment(segment_name);
}

std::vector<section_64*> KernelCache::read_fs_sections(const std::string& fileset_name, const std::string& segment_name) {
    auto* fileset = read_fileset(fileset_name);
    MachOBinary binary {data_, fileset->fileoff};
    return binary.read_sections(segment_name);
}

segment_command_64 *KernelCache::read_prelink_info_segment() {
    return MachOBinary{data_, 0}.read_segment("__PRELINK_INFO");
}
