//
// Created by Sreejith Krishnan R on 28/02/23.
//

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