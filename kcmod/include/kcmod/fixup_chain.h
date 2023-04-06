//
// Created by Sreejith Krishnan R on 26/02/23.
//

#pragma once

#include <mach-o/fixup-chains.h>
#include "macho.h"

namespace kcmod {

union DyldFixupPointer {
    struct {
        uint64_t raw
            : 51,
            next: 11,
            bind: 1,
            auth: 1;
    };

    dyld_chained_ptr_arm64e_rebase ptr_rebase;
    dyld_chained_ptr_arm64e_bind ptr_bind;
    dyld_chained_ptr_arm64e_auth_rebase ptr_auth_rebase;
    dyld_chained_ptr_arm64e_auth_bind ptr_auth_bind;
};

struct DyldChainedImport {
    dyld_chained_import import;
    std::string symbol_name;
};

class DyldFixupChainEditor {
public:
    DyldFixupChainEditor(MachOBinary<char> binary)
        : binary_(binary) {}

    void remove_fixups(uint64_t fileoff, uint64_t size);
    void add_fixup(uint64_t fileoff, DyldFixupPointer pointer);
    std::vector<DyldFixupPointer*> read_fixups();
    std::vector<DyldChainedImport> read_chained_imports();

private:
    dyld_chained_fixups_header* read_header();
    dyld_chained_starts_in_image* read_starts_in_image();
    std::vector<dyld_chained_starts_in_segment*> read_starts_in_segment();
    dyld_chained_starts_in_segment* find_fixup_segment(uint64_t fileoff, uint64_t* page_idx_out);
    DyldFixupPointer* find_fixup(uint64_t fileoff, DyldFixupPointer**previous_out);

private:
    MachOBinary<char> binary_;
};

}// namespace kcmod
