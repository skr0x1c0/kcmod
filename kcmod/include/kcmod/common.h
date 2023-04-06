//
// Created by Sreejith Krishnan R on 24/02/23.
//

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
