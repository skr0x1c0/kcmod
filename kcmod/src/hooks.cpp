//
// Created by Sreejith Krishnan R on 01/03/23.
//

#include "hooks.h"


using namespace kcmod;


KCModHookReader::KCModHookReader(std::span<const char> data, uint64_t offset)
    : data_{data}, offset_{offset} {
    MachOBinary binary{data_, offset_};
    for (const auto *segment: binary.read_segments()) {
        for (const auto *section: binary.read_sections(segment->segname)) {
            sections_.push_back(section);
        }
    }
}

std::vector<KCModHook> KCModHookReader::read_hooks() {
    MachOBinary binary{data_, offset_};
    std::map<std::string, HookEntry> hooks;
    for (const auto &[name, symbol]: binary.read_symbols()) {
        Symbol s {*symbol};
        if ((symbol->n_type & N_TYPE) != N_SECT) {
            continue;
        }
        if (!name.starts_with(k_hook_prefix)) {
            continue;
        }
        auto hook_name = name.substr(strlen(k_hook_prefix));
        kcmod_decode_verify(hook_name.size() != 0);

        bool is_super = hook_name.starts_with(k_hook_super_prefix);
        kcmod_decode_verify(is_super || hook_name.starts_with(k_hook_override_prefix));
        std::string fn_name = is_super ? hook_name.substr(strlen(k_hook_super_prefix) - 1)
                                       : hook_name.substr(strlen(k_hook_override_prefix) - 1);
        kcmod_decode_verify(fn_name.size() > 1);

        struct HookEntry *entry;
        if (auto it = hooks.find(fn_name); it != hooks.end()) {
            entry = &it->second;
        } else {
            entry = &hooks.insert({fn_name,
                                   {nullptr, nullptr}})
                         .first->second;
        }

        if (is_super) {
            kcmod_decode_verify(entry->fn_super == nullptr);
            entry->fn_super = symbol;
        } else {
            kcmod_decode_verify(entry->fn_override == nullptr);
            entry->fn_override = symbol;
        }
    }
    std::vector<KCModHook> result;
    for (const auto &[name, entry]: hooks) {
        kcmod_decode_verify(entry.fn_super != nullptr);
        kcmod_decode_verify(entry.fn_override != nullptr);
        result.emplace_back(KCModHook{
            .fn_name = name,
            .super_fn = process_hook_symbol(*entry.fn_super),
            .hook_fn = process_hook_symbol(*entry.fn_override),
        });
    }
    return result;
}

Symbol KCModHookReader::process_hook_symbol(const nlist_64 &entry) {
    Symbol symbol{entry};
    kcmod_decode_verify(symbol.type == Symbol::SECT);
    const auto *section = sections_[symbol.section - 1];
    kcmod_verify(section->sectname == std::string{"__text"});
    kcmod_verify(section->segname == std::string{"__TEXT_EXEC"});
    return symbol;
}
