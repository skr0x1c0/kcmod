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

#include <CoreFoundation/CFURL.h>
#include <mio/mmap.hpp>

#include "plist.h"
#include "common.h"


using namespace kcmod;


PropertyList::PropertyList(const std::filesystem::path &path)
    : PropertyList{std::span<const char>{mio::mmap_source {path.string()}}} {}

PropertyList::PropertyList(const std::span<const char> data) {
    CFDataRef cf_data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(data.data()), data.size(), kCFAllocatorNull);
    plist_ = CFPropertyListCreateFromXMLData(
        kCFAllocatorDefault,
        cf_data,
        kCFPropertyListMutableContainers,
        nullptr);
    CFRelease(cf_data);
    if (plist_ == nullptr) {
        throw DecodeError {"Failed to decode property list"};
    }
}

PropertyList::~PropertyList() {
    CFRelease(plist_);
}


