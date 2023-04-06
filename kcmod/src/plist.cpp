//
// Created by Sreejith Krishnan R on 25/02/23.
//

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


