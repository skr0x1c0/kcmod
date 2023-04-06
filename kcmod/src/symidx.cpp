//
// Created by Sreejith Krishnan R on 28/02/23.
//

#include "symidx.h"
#include "common.h"


using namespace kcmod;

Symbol::Symbol(const nlist_64 &nlist)
    : vmaddr{nlist.n_value},
      is_debug{(nlist.n_type & N_STAB) != 0},
      is_private_ext{(nlist.n_type & N_PEXT) != 0},
      is_external{(nlist.n_type & N_EXT) != 0},
      type{static_cast<Type>(nlist.n_type & N_TYPE)},
      section{nlist.n_sect} {}

void SymbolRegistry::index_binary(std::span<const char> data, uint64_t offset) {
    MachOBinary binary{data, offset};
    for (auto [name, nlist]: binary.read_symbols()) {
        if (name.size() == 0) {
            continue;
        }
        Symbol symbol{*nlist};
        if (auto it = symbols_.find(name); it != symbols_.end()) {
            it->second.push_back(symbol);
        } else {
            symbols_[name] = std::vector<Symbol>{symbol};
        }
    }
}

const std::vector<Symbol> *SymbolRegistry::find_registered_symbols(const std::string &name) const {
    if (auto it = symbols_.find(name); it != symbols_.end()) {
        return &it->second;
    }
    return nullptr;
}

Symbol SymbolRegistry::find_bind_symbol(const std::string &name) const {
    std::vector<Symbol> symbols;
    if (auto it = override_symbols_.find(name); it != override_symbols_.end()) {
        return {it->second};
    }
    if (auto it = symbols_.find(name); it != symbols_.end()) {
        for (const auto& symbol: it->second) {
            if (symbol.type == Symbol::UNDEF) {
                continue;
            }
            symbols.push_back(symbol);
        }
    }
    if (symbols.size() == 0) {
        throw FatalError {
            "Failed to find symbol {} in kernelcache",
            name,
        };
    }
    if (symbols.size() > 1) {
        // TODO: find the fileset containing the symbol
        throw FatalError {
            "Ambiguous symbol {} in kernelcache",
            name,
        };
    }
    return symbols[0];
}

void SymbolRegistry::add_override(const std::string &name, uint64_t address) {
    Symbol symbol;
    symbol.type = Symbol::ABS;
    symbol.is_external = true;
    symbol.is_private_ext = true;
    symbol.vmaddr = address;
    symbol.section = 0;
    symbol.is_debug = false;
    auto [_, inserted] = override_symbols_.insert({name, symbol});
    if (!inserted) {
        throw KeyExistError {
            "Symbol override already exists for {}", name
        };
    }
}
