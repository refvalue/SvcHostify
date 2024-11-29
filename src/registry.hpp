/*
 * Copyright (c) 2024 The RefValue Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <essence/abi/string.hpp>
#include <essence/zstring_view.hpp>

namespace essence::win {
    abi::string get_registry_string(std::string_view path, std::string_view name);
    std::vector<abi::string> get_registry_multi_string(std::string_view path, std::string_view name);
    std::vector<std::byte> get_registry_binary(std::string_view path, std::string_view name);
    std::uint32_t get_registry_dword(std::string_view path, std::string_view name);
    std::uint64_t get_registry_qword(std::string_view path, std::string_view name);

    void set_registry(std::string_view path, std::string_view name, std::span<const std::string> values);
    void set_registry(std::string_view path, std::string_view name, std::span<const std::byte> values);
    void set_registry(std::string_view path, std::string_view name, zstring_view value, bool expand_sz = false);
    void set_registry(std::string_view path, std::string_view name, std::uint32_t value);
    void set_registry(std::string_view path, std::string_view name, std::uint64_t value);
    void delete_registry(std::string_view path);
    void delete_registry(std::string_view path, std::string_view name);
} // namespace essence::win
