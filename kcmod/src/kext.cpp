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
