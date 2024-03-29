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

#include <cstdint>
#include <vector>

#include "debug.h"

#include "common.h"


namespace kcmod::aarch64 {

class BadInstruction : public FatalError {
    using FatalError::FatalError;
};

#define kcmod_aarch64_verify(condition) \
    if (!(condition)) { \
        throw kcmod::aarch64::BadInstruction{"verify condition {} failed at {}:{}", #condition, __FILE__, __LINE__}; \
    } \
    0

struct Adr {
    static constexpr auto k_max_imm = 1ULL << 20;

    Adr(uint32_t instr) {
        i_.raw = instr;
        kcmod_aarch64_verify(i_.op1 == 0b10000);
        kcmod_aarch64_verify(i_.op2 == 0);
    }

    uint32_t encode() {
        return i_.raw;
    }

    int64_t imm() {
        return sign_extend<21>((uint64_t)i_.immhi << 2 | i_.immlo);
    }

    void set_imm(int64_t offset) {
        kcmod_aarch64_verify(std::abs(offset) < k_max_imm);
        i_.immhi = offset >> 2;
        i_.immlo = offset;
    }

private:
    union {
        struct {
            uint32_t
                rd : 5,
                immhi: 19,
                op1: 5,
                immlo: 2,
                op2: 1;
        };
        uint32_t raw;
    } i_;
};

struct Adrp {
    static constexpr auto k_max_imm = 1ULL << 32;

    Adrp(uint32_t instr) {
        i_.raw = instr;
        kcmod_aarch64_verify(i_.op1 == 0b10000);
        kcmod_aarch64_verify(i_.op2 == 1);
    }

    uint32_t encode() {
        return i_.raw;
    }

    int64_t imm() {
        return sign_extend<33>(((uint64_t)i_.immhi << 2 | i_.immlo) << 12);
    }

    void set_imm(int64_t offset) {
        kcmod_aarch64_verify(std::abs(offset) < k_max_imm);
        kcmod_aarch64_verify(offset % 4096 == 0);
        i_.immhi = (offset >> 14) & 0x7FFFF;
        i_.immlo = (offset >> 12) & 0x3;
    }

private:
    union {
        struct {
            uint32_t
                rd : 5,
                immhi: 19,
                op1: 5,
                immlo: 2,
                op2: 1;
        };
        uint32_t raw;
    } i_;
};

struct AddImm {
    static constexpr auto k_max_imm = 1ULL << 12;

    AddImm(uint32_t instr) {
        i_.raw = instr;
        kcmod_aarch64_verify(i_.op == 0b00100010);
    }

    uint32_t encode() {
        return i_.raw;
    }

    uint64_t imm() {
        return i_.imm12;
    }

    void set_imm(uint64_t value) {
        kcmod_aarch64_verify(value < k_max_imm);
        i_.imm12 = value;
    }

private:
    union {
        struct {
            uint32_t
                rd : 5,
                rn: 5,
                imm12: 12,
                sh: 1,
                op: 8,
                sf: 1;
        };
        uint32_t raw;
    } i_;
};

struct Branch {
    static constexpr auto k_max_imm = 1ULL << 27;

    Branch(int64_t offset, bool link) {
        i_.raw = 0;
        i_.op = 0b00101;
        i_.link = link;
        set_imm(offset);
    }

    Branch(uint32_t instr) {
        i_.raw = instr;
        kcmod_aarch64_verify(i_.op == 0b000101);
    }

    uint32_t encode() {
        return i_.raw;
    }

    int64_t imm() {
        return sign_extend<28>((uint64_t)i_.imm26 << 2);
    }

    void set_imm(int64_t offset) {
        kcmod_aarch64_verify(std::abs(offset) < k_max_imm);
        kcmod_aarch64_verify(offset % 4 == 0);
        i_.imm26 = offset >> 2;
    }

private:
    union {
        struct {
            uint32_t
                imm26: 26,
                op: 5,
                link: 1;
        };
        uint32_t raw;
    } i_;
};

struct BranchCond {
    static constexpr auto k_max_imm = 1ULL << 20;

    BranchCond(uint32_t instr) {
        i_.raw = instr;
        kcmod_aarch64_verify(i_.op1 == 0);
        kcmod_aarch64_verify(i_.op2 == 0b0101010);
    }

    uint32_t encode() {
        return i_.raw;
    }

    int64_t imm() {
        return sign_extend<21>((uint64_t)i_.imm19 << 2);
    }

    void set_imm(int64_t offset) {
        kcmod_aarch64_verify(std::abs(offset) < k_max_imm);
        kcmod_aarch64_verify(offset % 4 == 0);
        i_.imm19 = offset >> 2;
    }

private:
    union {
        struct {
            uint32_t
                cond: 4,
                op1: 1,
                imm19: 19,
                op2: 8;
        };
        uint32_t raw;
    } i_;
};

struct BranchCompareZero {
    static constexpr auto k_max_imm = 1ULL << 20;

    BranchCompareZero(uint32_t instr) {
        i_.raw = instr;
        kcmod_aarch64_verify(i_.op == 0b011010);
    }

    uint32_t encode() {
        return i_.raw;
    }

    int64_t imm() {
        return sign_extend<21>((uint64_t)i_.imm19 << 2);
    }

    void set_imm(int64_t offset) {
        kcmod_aarch64_verify(std::abs(offset) < k_max_imm);
        kcmod_aarch64_verify(offset % 4 == 0);
        i_.imm19 = offset >> 2;
    }

private:
    union {
        struct {
            uint32_t
                rt: 5,
                imm19: 19,
                not_zero: 1,
                op: 6,
                sf: 1;
        };
        uint32_t raw;
    } i_;
};

struct BranchTestBitZero {
    static constexpr auto k_max_imm = 1ULL << 15;

    BranchTestBitZero(uint32_t instr) {
        i_.raw = instr;
        kcmod_aarch64_verify(i_.op == 0b011011);
    }

    uint32_t encode() {
        return i_.raw;
    }

    int64_t imm() {
        return sign_extend<16>((uint64_t)i_.imm14 << 2);
    }

    void set_imm(int64_t offset) {
        kcmod_aarch64_verify(std::abs(offset) < k_max_imm);
        kcmod_aarch64_verify(offset % 4 == 0);
        i_.imm14 = offset >> 2;
    }

private:
    union {
        struct {
            uint32_t
                rt: 5,
                imm14: 14,
                b40: 5,
                not_zero: 1,
                op: 6,
                b5: 1;
        };
        uint32_t raw;
    } i_;
};

struct LdrLiteral {
    static constexpr auto k_max_imm = 1ULL << 20;

    LdrLiteral(uint32_t instr) {
        i_.raw = instr;
        kcmod_aarch64_verify(i_.op1 == 0b011000);
        kcmod_aarch64_verify(i_.op2 == 0);
    }

    uint32_t encode() {
        return i_.raw;
    }

    int64_t imm() {
        return sign_extend<21>((uint64_t)i_.imm19 << 2);
    }

    void set_imm(int64_t offset) {
        kcmod_aarch64_verify(std::abs(offset) < k_max_imm);
        kcmod_aarch64_verify(offset % 4 == 0);
        i_.imm19 = offset >> 2;
    }

private:
    union {
        struct {
            uint32_t
                rt: 5,
                imm19: 19,
                op1: 6,
                opx: 1,
                op2: 1;
        };
        uint32_t raw;
    } i_;
};


struct LdrImmediate {
    static constexpr auto k_max_imm = 32760;

    LdrImmediate(uint32_t instr) {
        i_.raw = instr;
        kcmod_aarch64_verify(i_.opc == 0b01);
        kcmod_aarch64_verify(i_.op1 == 0b111001);
        kcmod_aarch64_verify(i_.size == 0b11); // 64-bit only
    }

    uint32_t encode() {
        return i_.raw;
    }

    uint64_t imm() {
        return (uint64_t)i_.imm12 << 3;
    }

    void set_imm(uint64_t offset) {
        kcmod_aarch64_verify(offset < k_max_imm);
        kcmod_aarch64_verify(offset % 8 == 0);
        i_.imm12 = offset >> 3;
    }

private:
    union {
        struct {
            uint32_t
                rt: 5,
                rn: 5,
                imm12: 12,
                opc: 2,
                op1: 6,
                size: 2;
        };
        uint32_t raw;
    } i_;
};


template <class T> std::optional<T> parse_instr(uint32_t raw_instr) {
    try {
        T instr{raw_instr};
        return instr;
    } catch (const BadInstruction&) {
        return std::nullopt;
    }
}

std::vector<uint32_t> build_hook_super_fn(uint32_t fn_instr, uint64_t fn_vmaddr, uint64_t super_fn_vmaddr);

static inline bool is_bti_instr(uint32_t instr) {
    return (instr & 0xffffff3f) == 0xd503241f;
}

}// namespace kcmod::aarch64