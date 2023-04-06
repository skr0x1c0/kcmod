//
// Created by Sreejith Krishnan R on 25/02/23.
//

#pragma once

#include <filesystem>

#include <CoreFoundation/CFPropertyList.h>

namespace kcmod {

class PropertyList {
public:
    PropertyList(const std::filesystem::path& path);
    PropertyList(const std::span<const char> data);
    CFPropertyListRef plist() { return plist_; };
    ~PropertyList();

private:
    CFPropertyListRef plist_;
};

}// namespace kcmod