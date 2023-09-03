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

#include "symidx.h"

namespace kcmod {

struct KCModHook {
    std::string fn_name;
    Symbol super_fn;
    Symbol hook_fn;
};

class KCModHookReader {
private:
    static constexpr const char* k_hook_prefix = "___kcmod_hook_";
    static constexpr const char* k_hook_super_prefix = "super_";
    static constexpr const char* k_hook_override_prefix = "override_";

    struct HookEntry {
        const nlist_64* fn_super;
        const nlist_64* fn_override;
    };

public:
    KCModHookReader(std::span<const char> data, uint64_t offset);
    std::vector<KCModHook> read_hooks();

private:
    Symbol process_hook_symbol(const nlist_64& nlist);

private:
    std::span<const char> data_;
    uint64_t offset_;
    std::vector<const section_64*> sections_;
};

}// namespace kcmod