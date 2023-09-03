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

#include <map>
#include <vector>

#include "macho.h"

namespace kcmod {

struct Symbol {
    uint64_t vmaddr;

    bool is_debug; // symbolic debugging entry
    bool is_private_ext;
    bool is_external;

    enum Type {
        UNDEF = 0x0,
        ABS = 0x2,
        SECT = 0xe,
        PBUD = 0xc,
        INDR = 0xa,
    } type;

    uint32_t section;

    Symbol() = default;
    Symbol(const nlist_64& nlist);
};

struct SymbolRegistry {
public:
    SymbolRegistry() = default;
    void index_binary(std::span<const char> data, uint64_t offset);
    void add_override(const std::string& name, uint64_t address);
    const std::vector<Symbol>* find_registered_symbols(const std::string& name) const;
    Symbol find_bind_symbol(const std::string& name) const;

private:
    std::map<std::string, std::vector<Symbol>> symbols_;
    std::map<std::string, Symbol> override_symbols_;
};

}// namespace kcmod