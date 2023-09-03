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
#include <span>
#include <vector>

#include <mach-o/loader.h>
#include <fmt/format.h>

namespace kcmod {

class GenericException : public std::exception {
public:
    template<typename... T>
    explicit GenericException(fmt::format_string<T...> fmt, T &&...args)
            : msg_{fmt::format(fmt, std::forward<T>(args)...)} {}
    [[nodiscard]] const char *what() const noexcept override { return msg_.c_str(); }

private:
    std::string msg_;
};

class FatalError : public GenericException {
    using GenericException::GenericException;
};

class DecodeError : public FatalError {
    using FatalError::FatalError;
};

#define kcmod_decode_verify(condition)                                                                                 \
    if (!(condition)) {                                                                                         \
        throw kcmod::DecodeError{"verify condition {} failed at {}:{}", #condition, __FILE__, __LINE__}; \
    }                                                                                                           \
    0

class KeyExistError : public FatalError {
    using FatalError::FatalError;
};


template <int bit, class T>
std::make_signed_t<T> sign_extend(T x) {
    static_assert(std::is_unsigned_v<T>);
    static_assert(bit > 0);
    static_assert(bit < sizeof(T) * 8);
    auto m = T(1) << (bit - 1);
    return (x ^ m) - m;
}

template <int bit, class T>
T round(T x) {
    static_assert(bit > 0);
    static_assert(bit < sizeof(T) * 8);
    auto m = (T(1) << bit) - 1;
    return x & ~m;
}

}// namespace kcmod
