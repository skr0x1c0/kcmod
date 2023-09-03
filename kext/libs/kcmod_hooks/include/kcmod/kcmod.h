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
