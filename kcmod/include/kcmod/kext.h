//
// Created by Sreejith Krishnan R on 25/02/23.
//

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