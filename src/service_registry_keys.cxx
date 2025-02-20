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

module refvalue.svchostify:service_registry_keys;
import essence.basic;
import std;

namespace essence::win {
    struct service_registry_keys {
        static constexpr std::string_view svchost_key{
            U8(R"(HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Svchost)")};

        static constexpr std::string_view service_param_key_pattern{
            U8(R"(HKLM\SYSTEM\CurrentControlSet\Services\{}\Parameters)")};

        static constexpr std::string_view co_initialize_security_param{U8("CoInitializeSecurityParam")};
        static constexpr std::string_view service_dll{U8("ServiceDll")};
        static constexpr std::string_view service_dll_unload_on_stop{U8("ServiceDllUnloadOnStop")};
        static constexpr std::string_view service_main{U8("ServiceMain")};
        static constexpr std::string_view startup_configuration{U8("StartupConfiguration")};
    };
} // namespace essence::win
