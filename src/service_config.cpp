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

#include "service_config.hpp"

#include "util.hpp"

#include <essence/char8_t_remediation.hpp>
#include <essence/crypto/digest.hpp>
#include <essence/json_compat.hpp>

using namespace essence::crypto;

namespace essence::win {
    service_config::logger_config service_config::default_values::logger_defaults::to_config() const {
        return {
            .base_path = base_path, .max_size = max_size, .max_files = max_files,
        };
    }

    [[nodiscard]] const service_config::default_values& service_config::defaults() {
        static const default_values defaults{
            .standalone        = true,
            .post_quit_message = false,
            .working_directory = get_executing_directory(),
            .dll_directories   = {{get_executing_directory()}},
            .logger =
                {
                    .base_path = U8("logs/svchostify.log"),
                    .max_size  = U8("50 MiB"),
                    .max_files = 5U,
                },
        };

        return defaults;
    }

    service_config service_config::from_msgpack_base64(std::string_view base64) {
        return json::from_msgpack(base64_decode(base64)).get<service_config>();
    }

    abi::string service_config::to_msgpack_base64() const {
        return base64_encode(json::to_msgpack(*this));
    }
} // namespace essence::win
