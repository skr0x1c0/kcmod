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

#pragma once

#include <optional>
#include <span>

#include <mach-o/loader.h>

#include "kext.h"
#include "plist.h"
#include "symidx.h"


namespace kcmod {

class KernelCache {
public:
    KernelCache(std::span<char> data) : data_{data} {}
    void replace_fileset(const std::string& fileset, const KernelExtension &binary,
                         const std::optional<std::filesystem::path>& symbols);

private:
    void replace_fileset_id(const std::string& from, const std::string& to);
    void replace_segment(const std::string& fileset, const KernelExtension& kext, const std::string& segment_name);
    void replace_text_segment(const std::string& fileset, const KernelExtension& kext);
    void apply_split_segment_fixups(const std::string& fileset, const KernelExtension& kext);

    void remove_prelink_info(const std::string& fileset);
    void insert_prelink_info(CFDictionaryRef info);
    void write_prelink_info(CFPropertyListRef plist);
    PropertyList read_prelink_info();

    fileset_entry_command* read_fileset(const std::string& fileset);
    std::vector<segment_command_64*> read_fs_segments(const std::string& fileset);
    segment_command_64* read_fs_segment(const std::string& fileset, const std::string& segment);
    std::vector<section_64*> read_fs_sections(const std::string& fileset, const std::string& segment);
    segment_command_64* read_prelink_info_segment();

    std::vector<std::string> read_kext_deps(const KernelExtension& kext);
    SymbolRegistry construct_symbol_registry(const KernelExtension& kext, const std::optional<std::filesystem::path>& symbols);

    void insert_kext_prelink_info(const KernelExtension& kext);
    void bind_kext_symbols(const KernelExtension& kext, const std::optional<std::filesystem::path>& symbols);
    void bind_hooks(const KernelExtension& kext, const std::optional<std::filesystem::path>& symbols);

private:
    std::span<char> data_;
};

}// namespace kcmod