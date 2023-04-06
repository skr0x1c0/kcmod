//
// Created by Sreejith Krishnan R on 28/02/23.
//

#pragma once

#include <string>

#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFString.h>


namespace kcmod::cf {

class String {
public:
    explicit String(CFStringRef ref): ref{ref} {
        CFRetain(ref);
    }

    explicit String(const std::string& value) {
        ref = CFStringCreateWithCString(kCFAllocatorDefault, value.c_str(), kCFStringEncodingUTF8);
    }

    std::string string() const {
        return std::string{CFStringGetCStringPtr(ref, kCFStringEncodingUTF8)};
    }

    ~String() {
        CFRelease(ref);
    }

public:
    CFStringRef ref;
};

class Dictionary;

class Array {
public:
    Array(CFArrayRef ref): ref{ref} {
        CFRetain(ref);
    }

    std::string read_string(size_t index);
    Dictionary read_dict(size_t index);
    size_t size();

    ~Array() {
        CFRelease(ref);
    }

public:
    CFArrayRef ref;
};


class Dictionary {
public:
    Dictionary(CFDictionaryRef ref) : ref{ref} {
        CFRetain(ref);
    }

    std::string read_string(const std::string &key);
    Array read_array(const std::string &key);
    Dictionary read_dict(const std::string &key);
    std::vector<std::string> keys();

    ~Dictionary() {
        CFRelease(ref);
    }

public:
    CFDictionaryRef ref;
};

std::string Dictionary::read_string(const std::string &key) {
    String key_cf{key};
    CFStringRef value = static_cast<CFStringRef>(CFDictionaryGetValue(ref, key_cf.ref));
    return String{value}.string();
}

std::vector<std::string> Dictionary::keys() {
    std::vector<CFStringRef> keys;
    keys.resize(CFDictionaryGetCount(ref));
    CFDictionaryGetKeysAndValues(ref, reinterpret_cast<const void**>(keys.data()), nullptr);
    std::vector<std::string> result;
    for (CFStringRef key : keys) {
        result.push_back(std::string {CFStringGetCStringPtr(key, kCFStringEncodingUTF8)});
    }
    return result;
}

std::string Array::read_string(size_t index) {
    CFStringRef value = static_cast<CFStringRef>(CFArrayGetValueAtIndex(ref, index));
    return String{value}.string();
}

size_t Array::size() {
    return CFArrayGetCount(ref);
}

Dictionary Array::read_dict(size_t index) {
    CFDictionaryRef value = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(ref, index));
    return Dictionary{value};
}

Array Dictionary::read_array(const std::string &key) {
    String key_cf{key};
    CFArrayRef value = static_cast<CFArrayRef>(CFDictionaryGetValue(ref, key_cf.ref));
    return Array{value};
}

Dictionary Dictionary::read_dict(const std::string &key) {
    String key_cf{key};
    CFDictionaryRef value = static_cast<CFDictionaryRef>(CFDictionaryGetValue(ref, key_cf.ref));
    return Dictionary{value};
}


}// namespace kcmod::cf
