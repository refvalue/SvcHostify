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

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <essence/abi/string.hpp>

namespace essence::win {
    struct service_config {
        enum class json_serialization {
            camel_case,
            enum_to_string,
        };

        struct logger_config {
            enum class json_serialization {
                camel_case,
                enum_to_string,
            };

            std::string base_path;
            std::optional<std::string> max_size;
            std::optional<std::size_t> max_files;
        };

        struct default_values {
            struct logger_defaults {
                std::string base_path;
                std::string max_size{};
                std::size_t max_files{};

                logger_config to_config() const;
            };

            bool standalone{};
            bool post_quit_message{};
            std::string working_directory;
            std::vector<std::string> dll_directories;
            logger_defaults logger;
        };

        service_worker_type worker_type{service_worker_type::executable};
        std::string name;
        std::string display_name;
        std::string context;
        service_account_type account_type{service_account_type::local_service};
        std::optional<bool> standalone;
        std::optional<bool> post_quit_message;
        std::optional<std::string> description;
        std::optional<std::string> jdk_directory;
        std::optional<std::string> working_directory;
        std::optional<std::vector<std::string>> arguments;
        std::optional<std::vector<std::string>> dll_directories;
        std::optional<logger_config> logger;

        [[nodiscard]] static const default_values& defaults();
        [[nodiscard]] static service_config from_msgpack_base64(std::string_view base64);
        [[nodiscard]] abi::string to_msgpack_base64() const;
    };
} // namespace essence::win
