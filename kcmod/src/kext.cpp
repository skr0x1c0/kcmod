//
// Created by Sreejith Krishnan R on 25/02/23.
//

#include <span>

#include "kext.h"
#include "plist.h"

using namespace kcmod;

namespace fs = std::filesystem;

KernelExtension::KernelExtension(const std::filesystem::path &path): path_{path} {
    bool is_macos_kext = path.extension() == ".kext";
    std::string kext_name = path.stem().string();
    binary_path_ = is_macos_kext ? path / "Contents" / "MacOS" / kext_name : path / kext_name;
    info_path_ = is_macos_kext ? path / "Contents" / "Info.plist" : path / "Info.plist";
    if (!fs::exists(binary_path_)) {
        throw DecodeError{"Mach-O binary not found at path {}", binary_path_.string()};
    }
    if (!fs::exists(info_path_)) {
        throw DecodeError{"Info.plist not found at path {}", info_path_.string()};
    }
    binary_mmap_ = mio::mmap_source{binary_path_.string()};
    binary_data_ = std::span{binary_mmap_.data(), binary_mmap_.size()};
    PropertyList plist{info_path_};
    CFStringRef bundle_id = static_cast<CFStringRef>(CFDictionaryGetValue(static_cast<CFDictionaryRef>(plist.plist()), CFSTR("CFBundleIdentifier")));
    bundle_id_ = CFStringGetCStringPtr(bundle_id, kCFStringEncodingUTF8);
}

PropertyList KernelExtension::read_info_plist() const {
    return PropertyList{info_path_};
}
