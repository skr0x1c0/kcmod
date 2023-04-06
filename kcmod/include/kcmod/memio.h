//
// Created by Sreejith Krishnan R on 25/02/23.
//

#pragma once

#include <fmt/format.h>
#include <span>

#include "debug.h"

namespace kcmod {

template <class CharType>
class SpanReader {
private:
    template <class T>
    using ReturnTypeT = std::conditional_t<std::is_const_v<CharType>, const T*, T*>;

public:
    SpanReader(std::span<CharType> data, uint64_t offset)
        : data_{data}, cursor_{0} {
        ensure_capacity(offset);
        cursor_ = offset;
    }

    template<class T>
    ReturnTypeT<T> peek() {
        ensure_capacity(sizeof(T));
        auto *ptr = reinterpret_cast<ReturnTypeT<T>>(
            data_.data() + cursor_);
        return ptr;
    }

    template<class T>
    ReturnTypeT<T> read() {
        auto *ptr = peek<T>();
        cursor_ += sizeof(T);
        return ptr;
    }

    std::span<CharType> peek_data(size_t size) {
        ensure_capacity(size);
        return data_.subspan(cursor_, size);
    }

    std::span<CharType> read_data(size_t size) {
        auto result = peek_data(size);
        cursor_ += result.size();
        return result;
    }

    std::string read_string() {
        ensure_capacity(1);
        auto start = data_.begin() + cursor_;
        auto end = std::find(data_.begin() + cursor_, data_.end(), '\0');
        kcmod_verify(end != data_.end());
        return std::string{start, end};
    }

    uint64_t read_uleb128() {
        uint64_t result = 0;
        int bit = 0;
        while (true) {
            uint8_t byte = *read<uint8_t>();
            uint64_t slice = byte & 0x7f;
            kcmod_verify(bit < 64);
            kcmod_verify(slice << bit >> bit == slice);
            result |= (slice << bit);
            bit += 7;
            if ((byte & 0x80) == 0) {
                return result;
            }
        }
    }

    void seek(size_t by) {
        ensure_capacity(by);
        cursor_ += by;
    }

    uint64_t cursor() const {
        return cursor_;
    }

protected:
    void ensure_capacity(uint64_t size) {
        if (cursor_ + size > data_.size()) {
            throw std::out_of_range{
                fmt::format(
                    "SpanReader::ensure_capacity for size {} failed at {}",
                    size, cursor_)};
        }
    }

protected:
    std::span<CharType> data_;
    uint64_t cursor_;
};

class SpanWriter: public SpanReader<char> {
public:
    SpanWriter(std::span<char> data, uint64_t offset)
        : SpanReader<char>(data, offset) {}

    template<class T>
    void put(const T &value) {
        ensure_capacity(sizeof(T));
        auto *ptr = reinterpret_cast<T *>(data_.data() + cursor_);
        *ptr = value;
    }

    template<class T>
    void write(const T &value) {
        put(value);
        cursor_ += sizeof(T);
    }

    void put(std::span<const char> data) {
        ensure_capacity(data.size());
        std::copy(data.begin(), data.end(), data_.begin() + cursor_);
    }

    void write(std::span<const char> data) {
        put(data);
        cursor_ += data.size();
    }

    void put_zero(size_t count) {
        ensure_capacity(count);
        memset(&data_[cursor_], 0, count);
    }

    void write_zero(size_t count) {
        put_zero(count);
        cursor_ += count;
    }

    void seek(size_t by) {
        ensure_capacity(by);
        cursor_ += by;
    }

    uint64_t cursor() const {
        return cursor_;
    }
};

}// namespace kcmod