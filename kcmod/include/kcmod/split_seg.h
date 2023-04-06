//
// Created by Sreejith Krishnan R on 26/02/23.
//

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
