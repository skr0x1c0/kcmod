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

#include "aarch64.h"

using namespace kcmod;
using namespace aarch64;

namespace {

template<class T>
std::optional<T> decode(uint32_t instr) {
    try {
        return T{instr};
    } catch (const BadInstruction &) {
        return std::nullopt;
    }
}

}// namespace

std::vector<uint32_t> aarch64::build_hook_super_fn(uint32_t fn_instr, uint64_t fn_vmaddr, uint64_t super_fn_vmaddr) {
    kcmod_verify(fn_vmaddr % 4 == 0);
    kcmod_verify(super_fn_vmaddr % 4 == 0);
    int64_t fn_offset = fn_vmaddr - super_fn_vmaddr;
    uint32_t super_tramp_instr = Branch{fn_offset, false}.encode();
    if (auto instr = decode<Adrp>(fn_instr)) {
        int64_t offset = instr->imm() + round<12>(fn_vmaddr) - round<12>(super_fn_vmaddr);
        if (std::abs(offset) < Adrp::k_max_imm) {
            instr->set_imm(offset);
            return {instr->encode(), super_tramp_instr};
        }
        kcmod_todo();
    }
    if (auto instr = decode<Adr>(fn_instr)) {
        int64_t offset = instr->imm() + fn_vmaddr - super_fn_vmaddr;
        if (std::abs(offset) < Adr::k_max_imm) {
            instr->set_imm(offset);
            return {instr->encode(), super_tramp_instr};
        }
        kcmod_todo();
    }
    if (auto instr = decode<Branch>(fn_instr)) {
        int64_t offset = instr->imm() + fn_vmaddr - super_fn_vmaddr;
        if (std::abs(offset) < Branch::k_max_imm) {
            instr->set_imm(offset);
            return {instr->encode(), super_tramp_instr};
        }
        kcmod_todo();
    }
    if (auto instr = decode<BranchCond>(fn_instr)) {
        int64_t offset = instr->imm() + fn_vmaddr - super_fn_vmaddr;
        if (std::abs(offset) < BranchCond::k_max_imm) {
            instr->set_imm(offset);
            return {instr->encode(), super_tramp_instr};
        }
        instr->set_imm(8);
        return {
            instr->encode(),
            super_tramp_instr,
            Branch {offset - 8, false}.encode(),
        };
    }
    if (auto instr = decode<BranchCompareZero>(fn_instr)) {
        int64_t offset = instr->imm() + fn_vmaddr - super_fn_vmaddr;
        if (std::abs(offset) < BranchCompareZero::k_max_imm) {
            instr->set_imm(offset);
            return {instr->encode(), super_tramp_instr};
        }
        instr->set_imm(8);
        return {
            instr->encode(),
            super_tramp_instr,
            Branch {offset - 8, false}.encode(),
        };
    }
    if (auto instr = decode<BranchTestBitZero>(fn_instr)) {
        int64_t offset = instr->imm() + fn_vmaddr - super_fn_vmaddr;
        if (std::abs(offset) < BranchTestBitZero::k_max_imm) {
            instr->set_imm(offset);
            return {instr->encode(), super_tramp_instr};
        }
        instr->set_imm(8);
        return {
            instr->encode(),
            super_tramp_instr,
            Branch {offset - 8, false}.encode(),
        };
    }
    if (auto instr = decode<LdrLiteral>(fn_instr)) {
        int64_t offset = instr->imm() + fn_vmaddr - super_fn_vmaddr;
        if (std::abs(offset) < LdrLiteral::k_max_imm) {
            instr->set_imm(offset);
            return {instr->encode(), super_tramp_instr};
        }
        kcmod_todo();
    }
    return std::vector<uint32_t>{fn_instr, super_tramp_instr};
}