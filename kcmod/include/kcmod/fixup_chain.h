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
