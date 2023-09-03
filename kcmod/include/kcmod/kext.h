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

#include <filesystem>

#include <mach-o/loader.h>
#include <mio/mmap.hpp>

#include "macho.h"
#include "plist.h"

namespace kcmod {

class KernelExtension {
public:
    KernelExtension(const std::filesystem::path &path);

    const std::string &bundle_id() const { return bundle_id_; }
    MachOBinary<const char> binary() const { return MachOBinary{binary_data_, 0}; }
    std::span<const char> binary_data() const { return binary_data_; }

    const segment_command_64 *read_segment(const std::string &name) const {
        return binary().read_segment(name);
    }

    std::vector<const segment_command_64 *> read_segments() const {
        return binary().read_segments();
    }

    std::vector<const section_64 *> read_sections(const std::string &segment) const {
        return binary().read_sections(segment);
    }

    PropertyList read_info_plist() const;

    const std::filesystem::path& path() const { return path_; }
    const std::filesystem::path& binary_path() const { return binary_path_; }
    const std::filesystem::path& info_path() const { return info_path_; }

private:
    std::filesystem::path path_;
    std::filesystem::path binary_path_;
    std::filesystem::path info_path_;
    std::string bundle_id_;
    mio::mmap_source binary_mmap_;
    std::span<const char> binary_data_;
};

}// namespace kcmod