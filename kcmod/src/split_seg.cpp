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

#include "split_seg.h"
#include "macho.h"


using namespace kcmod;

namespace {

class SplitSegmentInfoParser {
public:
    SplitSegmentInfoParser(std::span<const char> data) : data_{data} {}

    std::vector<DyldCacheAdjV2Entry> parse() {
        std::vector<DyldCacheAdjV2Entry> result;
        MachOBinary binary{data_};
        const auto *cmd = binary.read_command<linkedit_data_command>(LC_SEGMENT_SPLIT_INFO);
        SpanReader reader{data_, cmd->dataoff};
        uint8_t version = *reader.read<uint8_t>();
        kcmod_verify(version == DYLD_CACHE_ADJ_V2_FORMAT);
        uint64_t section_count = reader.read_uleb128();
        for (uint64_t section_idx = 0; section_idx < section_count; ++section_idx) {
            uint64_t from_section_idx = reader.read_uleb128();
            uint64_t to_section_idx = reader.read_uleb128();
            uint64_t to_offset_count = reader.read_uleb128();
            uint64_t to_section_offset = 0;
            for (uint64_t to_offset_idx = 0; to_offset_idx < to_offset_count; ++to_offset_idx) {
                to_section_offset += reader.read_uleb128();
                uint64_t from_offset_count = reader.read_uleb128();
                for (uint64_t from_offset_idx = 0; from_offset_idx < from_offset_count; ++from_offset_idx) {
                    uint64_t kind = reader.read_uleb128();
                    kcmod_verify(kind < (uint64_t) DyldCacheAdjV2Kind::Invalid);
                    uint64_t from_section_delta_count = reader.read_uleb128();
                    uint64_t from_section_offset = 0;
                    for (uint64_t from_section_delta_idx = 0; from_section_delta_idx < from_section_delta_count; ++from_section_delta_idx) {
                        from_section_offset += reader.read_uleb128();
                        result.push_back(DyldCacheAdjV2Entry{
                            .from_section_idx = from_section_idx,
                            .to_section_idx = to_section_idx,
                            .from_section_offset = from_section_offset,
                            .to_section_offset = to_section_offset,
                            .kind = static_cast<DyldCacheAdjV2Kind>(kind)});
                    }
                }
            }
        }
        return result;
    }

private:
    std::span<const char> data_;
};

}// namespace


std::vector<DyldCacheAdjV2Entry> kcmod::parse_split_seg_info(std::span<const char> data) {
    return SplitSegmentInfoParser{data}.parse();
}
