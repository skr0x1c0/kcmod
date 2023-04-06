//
// Created by Sreejith Krishnan R on 25/02/23.
//

#pragma once

#include <map>
#include <span>
#include <vector>

#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/stab.h>

#include "common.h"
#include "memio.h"


namespace kcmod {

template <class CharType = const char>
class MachOBinary {
private:
    template <class T>
    using ReturnTypeT = std::conditional_t<std::is_const_v<CharType>, const T*, T*>;

public:
    MachOBinary(std::span<CharType> data, uint64_t offset = 0)
        : data_{data}, offset_{offset} {
        SpanReader reader{data, offset};
        auto *header = reader.template read<mach_header_64>();
        kcmod_decode_verify(header->magic == MH_MAGIC_64);
    }

    template <class T>
    std::vector<ReturnTypeT<T>> read_commands(uint32_t command) {
        std::vector<ReturnTypeT<T>> result;
        SpanReader reader {data_, offset_};
        auto *header = reader.template read<mach_header_64>();
        for (size_t i=0; i<header->ncmds; ++i) {
            auto* cmd = reader.template peek<load_command>();
            if (cmd->cmd == command) {
                result.push_back(reader.template peek<T>());
            }
            reader.seek(cmd->cmdsize);
        }
        return result;
    }

    template <class T>
    ReturnTypeT<T> read_command(uint32_t command) {
        std::vector<ReturnTypeT<T>> commands = read_commands<T>(command);
        kcmod_decode_verify(commands.size() <= 1);
        if (commands.empty()) {
            return nullptr;
        }
        return commands[0];
    }

    std::map<std::string, ReturnTypeT<fileset_entry_command>> read_filesets() {
        std::map<std::string, ReturnTypeT<fileset_entry_command>> result;
        SpanReader reader {data_, offset_};
        auto *header = reader.template read<mach_header_64>();
        for (size_t i=0; i<header->ncmds; ++i) {
            auto* cmd = reader.template peek<load_command>();
            if (cmd->cmd == LC_FILESET_ENTRY) {
                auto* fileset = reader.template peek<fileset_entry_command>();
                SpanReader str_reader{data_, reader.cursor() + fileset->entry_id.offset};
                result.emplace(str_reader.read_string(), fileset);
            }
            reader.seek(cmd->cmdsize);
        }
        return result;
    }

    std::optional<uint32_t> find_fileset_index(const std::string& name) {
        SpanReader reader {data_, offset_};
        auto *header = reader.template read<mach_header_64>();
        for (size_t i=0; i<header->ncmds; ++i) {
            auto* cmd = reader.template peek<load_command>();
            if (cmd->cmd == LC_FILESET_ENTRY) {
                auto* fileset = reader.template peek<fileset_entry_command>();
                SpanReader str_reader{data_, reader.cursor() + fileset->entry_id.offset};
                if (str_reader.read_string() == name) {
                    return i;
                }
            }
            reader.seek(cmd->cmdsize);
        }
        return std::nullopt;
    }

    std::vector<ReturnTypeT<segment_command_64>> read_segments() {
        return read_commands<segment_command_64>(LC_SEGMENT_64);
    }

    ReturnTypeT<segment_command_64> read_segment(const std::string& segment_name) {
        std::vector<ReturnTypeT<segment_command_64>> segments = read_segments();
        for (auto* segment: segments) {
            if (std::string{segment->segname} == segment_name) {
                return segment;
            }
        }
        return nullptr;
    }

    std::vector<ReturnTypeT<section_64>> read_sections(const std::string& segment_name) {
        auto* segment = read_segment(segment_name);
        SpanReader reader{data_, static_cast<uint64_t>((CharType*)segment - data_.data())};
        reader.seek(sizeof(*segment));
        std::vector<ReturnTypeT<section_64>> result;
        for (size_t i=0; i<segment->nsects; ++i) {
            result.push_back(reader.template read<section_64>());
        }
        return result;
    }

    uint64_t read_used_segment_size(const std::string& segment_name) {
        uint64_t max_offset = 0;
        for (const section_64* section: read_sections(segment_name)) {
            max_offset = std::max(max_offset, section->offset + section->size);
        }
        const auto* segment = read_segment(segment_name);
        return max_offset - segment->fileoff;
    }

    uint64_t vm_base() {
        for (const auto* segment: read_segments()) {
            if (segment->fileoff == 0) {
                return segment->vmaddr;
            }
        }
        kcmod_not_reachable();
    }

    std::span<CharType> data() {
        return data_;
    }

    ReturnTypeT<segment_command_64> find_segment_with_va(uint64_t vmaddr) {
        for (auto* segment: read_segments()) {
            if (segment->vmaddr <= vmaddr && vmaddr < segment->vmaddr + segment->vmsize) {
                return segment;
            }
        }
        return nullptr;
    }

    ReturnTypeT<segment_command_64> find_segment_with_fileoff(uint64_t fileoff) {
        for (auto* segment: read_segments()) {
            if (segment->fileoff <= fileoff && fileoff < segment->fileoff + segment->filesize) {
                return segment;
            }
        }
        return nullptr;
    }

    uint64_t fileoff_from_pointer(const char* ptr) {
        kcmod_verify(ptr >= data_.data() && ptr < data_.data() + data_.size());
        return ptr - data_.data();
    }

    ReturnTypeT<segment_command_64> find_segment_with_pointer(const char* ptr) {
        return find_segment_with_fileoff(fileoff_from_pointer(ptr));
    }

    ReturnTypeT<uuid_command> read_uuid() {
        return read_command<uuid_command>(LC_UUID);
    }

    std::vector<std::pair<std::string, ReturnTypeT<nlist_64>>> read_symbols() {
        const symtab_command* cmd = read_command<symtab_command>(LC_SYMTAB);
        SpanReader reader{data_, cmd->symoff};
        std::vector<std::pair<std::string, ReturnTypeT<nlist_64>>> result;
        for (size_t i=0; i<cmd->nsyms; ++i) {
            auto* nlist = reader.template read<nlist_64>();
            SpanReader str_reader{data_, cmd->stroff + nlist->n_un.n_strx};
            result.push_back(std::pair{
                str_reader.read_string(),
                nlist
            });
        }
        return result;
    }

private:
    std::span<CharType> data_;
    uint64_t offset_;
};

}// namespace kcmod