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

#include <span>

namespace kcmod {

#define DYLD_CACHE_ADJ_V2_FORMAT				0x7F

enum class DyldCacheAdjV2Kind {
    Pointer32 = 0x01,
    Pointer64 = 0x02,
    Delta32 = 0x03,
    Delta64 = 0x04,
    Arm64Adrp = 0x05,
    Arm64Off12 = 0x06,
    Arm64Br26 = 0x07,
    ArmMovwMovt = 0x08,
    ArmBr24 = 0x09,
    ThumbMovwMovt = 0x0A,
    ThumbBr22 = 0x0B,
    ImageOff32 = 0x0C,
    ThreadedPointer64 = 0x0D,
    Invalid = 0x0E
};

struct DyldCacheAdjV2Entry {
    uint64_t from_section_idx;
    uint64_t to_section_idx;
    uint64_t from_section_offset;
    uint64_t to_section_offset;
    DyldCacheAdjV2Kind kind;
};

std::vector<DyldCacheAdjV2Entry> parse_split_seg_info(std::span<const char> data);

}// namespace kcmod
