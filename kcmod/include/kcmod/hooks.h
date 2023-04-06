//
// Created by Sreejith Krishnan R on 01/03/23.
//

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