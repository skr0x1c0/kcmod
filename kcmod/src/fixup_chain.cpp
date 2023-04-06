//
// Created by Sreejith Krishnan R on 26/02/23.
//

#include <mach-o/fixup-chains.h>

#include "fixup_chain.h"
#include "log.h"


using namespace kcmod;


dyld_chained_fixups_header *DyldFixupChainEditor::read_header() {
    const auto *cmd = binary_.read_command<linkedit_data_command>(LC_DYLD_CHAINED_FIXUPS);
    kcmod_decode_verify(cmd != nullptr);
    SpanReader reader(binary_.data(), cmd->dataoff);
    return reader.peek<dyld_chained_fixups_header>();
}

dyld_chained_starts_in_image *DyldFixupChainEditor::read_starts_in_image() {
    const auto *cmd = binary_.read_command<linkedit_data_command>(LC_DYLD_CHAINED_FIXUPS);
    kcmod_decode_verify(cmd != nullptr);

    SpanReader reader(binary_.data(), cmd->dataoff);
    const auto *header = reader.peek<dyld_chained_fixups_header>();
    reader.seek(header->starts_offset);

    auto *starts_in_image = reader.peek<dyld_chained_starts_in_image>();
    reader.peek_data(
        offsetof(dyld_chained_starts_in_image, seg_info_offset) +
        sizeof(dyld_chained_starts_in_image::seg_info_offset[0]) * starts_in_image->seg_count);
    return starts_in_image;
}

std::vector<dyld_chained_starts_in_segment *> DyldFixupChainEditor::read_starts_in_segment() {
    const auto *cmd = binary_.read_command<linkedit_data_command>(LC_DYLD_CHAINED_FIXUPS);
    const auto *starts_in_image = read_starts_in_image();
    const auto *header = read_header();
    std::vector<dyld_chained_starts_in_segment *> result;
    for (size_t i = 0; i < starts_in_image->seg_count; ++i) {
        uint32_t seg_info_offset = starts_in_image->seg_info_offset[i];
        if (seg_info_offset == 0) {
            continue;
        }
        SpanReader reader{binary_.data(), cmd->dataoff + header->starts_offset + seg_info_offset};
        auto *starts_in_segment = reader.peek<dyld_chained_starts_in_segment>();
        kcmod_decode_verify(
            starts_in_segment->pointer_format == DYLD_CHAINED_PTR_64_KERNEL_CACHE ||
            starts_in_segment->pointer_format == DYLD_CHAINED_PTR_ARM64E_KERNEL);
        reader.peek_data(
            offsetof(dyld_chained_starts_in_segment, page_start) +
            sizeof(dyld_chained_starts_in_segment::page_start[0]) * starts_in_segment->page_count);
        result.push_back(starts_in_segment);
    }
    return result;
}

dyld_chained_starts_in_segment *DyldFixupChainEditor::find_fixup_segment(uint64_t fileoff, uint64_t *page_idx_out) {
    std::vector<dyld_chained_starts_in_segment *> seg_starts = read_starts_in_segment();
    for (auto *start: seg_starts) {
        if (fileoff < start->segment_offset) {
            continue;
        }
        uint64_t segoff = fileoff - start->segment_offset;
        uint64_t page_idx = segoff / start->page_size;
        if (page_idx > start->page_count) {
            continue;
        }
        if (page_idx_out != nullptr) {
            *page_idx_out = page_idx;
        }
        return start;
    }
    return nullptr;
}

DyldFixupPointer *DyldFixupChainEditor::find_fixup(uint64_t fileoff, DyldFixupPointer **previous_out) {
    uint64_t page_idx = 0;
    dyld_chained_starts_in_segment *segment = find_fixup_segment(fileoff, &page_idx);
    if (segment == nullptr) {
        return nullptr;
    }
    if (segment->page_start[page_idx] == DYLD_CHAINED_PTR_START_NONE) {
        return nullptr;
    }
    uint64_t fixup_page_start = segment->segment_offset + page_idx * segment->page_count + segment->page_start[page_idx];
    SpanReader reader{binary_.data(), fixup_page_start};
    DyldFixupPointer *prev = nullptr;
    do {
        auto *ptr = reader.peek<DyldFixupPointer>();
        if (reader.cursor() != fileoff) {
            prev = ptr;
            reader.seek(ptr->next * 4);
            continue;
        }
        if (previous_out != nullptr) {
            *previous_out = prev;
        }
        return ptr;
    } while (prev->next != 0);
    return nullptr;
}

void DyldFixupChainEditor::remove_fixups(uint64_t fileoff, uint64_t size) {
    uint64_t end = fileoff + size;
    size_t removed = 0;
    for (uint64_t cursor = (fileoff / 16384) * 16384; cursor < end; cursor += 16384) {// TODO: dynamic page size
        uint64_t page_idx = 0;
        auto *fixup_segment = find_fixup_segment(cursor, &page_idx);
        if (fixup_segment == nullptr) {
            continue;
        }
        kcmod_verify(fixup_segment->page_size == 16384);
        kcmod_verify(
            fixup_segment->pointer_format == DYLD_CHAINED_PTR_64_KERNEL_CACHE ||
            fixup_segment->pointer_format == DYLD_CHAINED_PTR_ARM64E_KERNEL);
        uint64_t page_start = fixup_segment->segment_offset + page_idx * fixup_segment->page_size + fixup_segment->page_start[page_idx];
        uint64_t page_end = fixup_segment->segment_offset + (page_idx + 1) * fixup_segment->page_size;
        if (page_start >= fileoff && page_end <= end) {
            fixup_segment->page_start[page_idx] = DYLD_CHAINED_PTR_START_NONE;
            continue;
        }
        SpanReader reader{binary_.data(), page_start};
        DyldFixupPointer *prev = nullptr;
        while (true) {
            auto *pointer = reader.peek<DyldFixupPointer>();
            if (reader.cursor() >= fileoff && reader.cursor() < end) {
                if (prev != nullptr) {
                    prev->next += pointer->next;
                } else {
                    fixup_segment->page_start[page_idx] += pointer->next * 4;
                }
                removed++;
            } else {
                prev = pointer;
            }
            if (pointer->next == 0) {
                break;
            }
            reader.seek(pointer->next * 4);
        }
    }
    kcmod_log_debug("removed {} fixups", removed);
}

void DyldFixupChainEditor::add_fixup(uint64_t fileoff, DyldFixupPointer pointer) {
    uint64_t page_idx = 0;
    dyld_chained_starts_in_segment *segment = find_fixup_segment(fileoff, &page_idx);
    kcmod_verify(segment != nullptr);
    if (segment->page_start[page_idx] == DYLD_CHAINED_PTR_START_NONE) {
        segment->page_start[page_idx] = fileoff - segment->segment_offset - page_idx * segment->page_size;
        pointer.next = 0;
        SpanWriter writer{binary_.data(), fileoff};
        writer.write(pointer);
        return;
    }
    uint64_t fixup_page_start = segment->segment_offset + page_idx * segment->page_size + segment->page_start[page_idx];
    SpanReader reader{binary_.data(), fixup_page_start};
    while (true) {
        auto *cursor = reader.peek<DyldFixupPointer>();
        kcmod_verify(reader.cursor() != fileoff);
        if (cursor->next == 0 || reader.cursor() + cursor->next * 4 > fileoff) {
            uint64_t next = (fileoff - reader.cursor()) / 4;
            kcmod_verify(next < DYLD_CHAINED_PTR_START_NONE);
            pointer.next = cursor->next > 0 ? cursor->next - next : 0;
            cursor->next = next;
            SpanWriter writer{binary_.data(), fileoff};
            writer.write(pointer);
            return;
        }
        reader.seek(cursor->next * 4);
    }
    kcmod_not_reachable();
}

std::vector<DyldFixupPointer *> DyldFixupChainEditor::read_fixups() {
    std::vector<DyldFixupPointer *> result;
    std::vector<dyld_chained_starts_in_segment *> seg_starts = read_starts_in_segment();
    for (auto *start: seg_starts) {
        for (uint64_t page_idx = 0; page_idx < start->page_count; page_idx++) {
            if (start->page_start[page_idx] == DYLD_CHAINED_PTR_START_NONE) {
                continue;
            }
            uint64_t fixup_page_start = start->segment_offset + page_idx * start->page_count + start->page_start[page_idx];
            SpanReader reader{binary_.data(), fixup_page_start};
            while (true) {
                auto *pointer = reader.peek<DyldFixupPointer>();
                result.push_back(pointer);
                if (pointer->next == 0) {
                    break;
                }
                reader.seek(pointer->next * 4);
            }
        }
    }
    return result;
}

std::vector<DyldChainedImport> DyldFixupChainEditor::read_chained_imports() {
    const auto* cmd = binary_.read_command<linkedit_data_command>(LC_DYLD_CHAINED_FIXUPS);
    const auto* header = read_header();
    kcmod_decode_verify(header->imports_format == DYLD_CHAINED_IMPORT);
    kcmod_decode_verify(header->symbols_format == 0);

    std::vector<DyldChainedImport> result;
    SpanReader imports_reader{binary_.data(), cmd->dataoff + header->imports_offset};
    for (size_t i=0; i<header->imports_count; ++i) {
        const auto* import = imports_reader.read<dyld_chained_import>();
        SpanReader symbols_reader{binary_.data(), cmd->dataoff + header->symbols_offset + import->name_offset};
        std::string name = symbols_reader.read_string();
        result.push_back(DyldChainedImport{
            .import = *import,
            .symbol_name = name
        });
    }
    return result;
}
