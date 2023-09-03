//
// Created by Sreejith Krishnan R on 22/02/23.
//

#pragma once


#define KCMOD_OVERRIDE(return_type, fn, ...)                                  \
    extern return_type fn(__VA_ARGS__);                                       \
    __attribute__((naked)) return_type __kcmod_hook_super_##fn(__VA_ARGS__) { \
        __asm__(                                                              \
            "brk 1\n"                                                         \
            "brk 1\n"                                                         \
            "brk 1\n"                                                         \
            "brk 1\n");                                                       \
    }                                                                         \
    return_type __kcmod_hook_override_##fn(__VA_ARGS__)


#define KCMOD_SUPER(fn, ...) __kcmod_hook_super_##fn(__VA_ARGS__)
