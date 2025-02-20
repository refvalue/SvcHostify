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

module;

#include <essence/char8_t_remediation.hpp>

module refvalue.svchostify;
import :config_setup;
import :registry;
import :service_registry_keys;

namespace essence::win {
    abstract::service_worker make_executable_service_worker(service_config config);
    abstract::service_worker make_pure_c_service_worker(service_config config);
    abstract::service_worker make_com_service_worker(service_config config);
    abstract::service_worker make_jvm_service_worker(service_config config);

    abstract::service_worker make_service_worker(service_config config) {
        switch (config.worker_type) {
        case service_worker_type::executable:
            return make_executable_service_worker(std::move(config));
        case service_worker_type::pure_c:
            return make_pure_c_service_worker(std::move(config));
        case service_worker_type::com:
            return make_com_service_worker(std::move(config));
        case service_worker_type::jvm:
            return make_jvm_service_worker(std::move(config));
        default:
            throw formatted_runtime_error{U8("Invalid worker type.")};
        }
    }

    abstract::service_worker make_service_worker_from_registry(zwstring_view service_name) {
        auto config = service_config::from_msgpack_base64(
            get_registry_string(format(service_registry_keys::service_param_key_pattern, to_utf8_string(service_name)),
                service_registry_keys::startup_configuration));

        setup_config(config, true);

        return make_service_worker(std::move(config));
    }
} // namespace essence::win
