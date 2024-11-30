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

#include "common_types.hpp"
#include "service_config.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <essence/abi/string.hpp>
#include <essence/zstring_view.hpp>

namespace essence::win {
    [[nodiscard]] abi::string get_system_error(std::uint32_t code);
    [[nodiscard]] abi::string get_last_error();
    [[nodiscard]] abi::string get_system_directory();
    [[nodiscard]] abi::string get_process_path();
    [[nodiscard]] abi::string get_executing_path();
    [[nodiscard]] std::string get_executing_directory();
    [[nodiscard]] std::uint32_t get_session_id();
    [[nodiscard]] zwstring_view get_service_account_name(service_account_type type);
    [[nodiscard]] error_checking_handler make_service_error_checker(const service_config& config);
    [[nodiscard]] std::vector<abi::string> parse_command_line(zwstring_view command_line);
    [[nodiscard]] abi::wstring make_command_line(std::span<const std::string> args);
    void allocate_console_and_redirect();
    void add_dll_directories(std::span<const std::string> directories);
} // namespace essence::win
