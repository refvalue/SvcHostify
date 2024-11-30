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

#include "util.hpp"

#include "filesystem_tokens.hpp"

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <memory>
#include <ranges>
#include <utility>

#include <essence/char8_t_remediation.hpp>
#include <essence/encoding.hpp>
#include <essence/error_extensions.hpp>
#include <essence/range.hpp>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI

#include <Windows.h>
#include <shellapi.h>

namespace essence::win {
    namespace {
        template <typename T>
        using local_ptr = std::unique_ptr<T[], decltype([](T* inner) { LocalFree(inner); })>;

        abi::string get_module_path(std::uint32_t flags, const void* name_or_address) {
            if (HMODULE module; GetModuleHandleExW(flags, static_cast<const wchar_t*>(name_or_address), &module)) {
                std::wstring path(MAX_PATH, L'\0');

                path.resize(GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size())));
                path.shrink_to_fit();

                return to_utf8_string(path);
            }

            return {};
        }
    } // namespace

    abi::string get_system_error(std::uint32_t code) {
        wchar_t* buffer{};
        const auto size = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);

        const local_ptr<wchar_t> message{buffer};

        return to_utf8_string(std::wstring_view{message.get(), size});
    }

    abi::string get_last_error() {
        return get_system_error(GetLastError());
    }

    abi::string get_system_directory() {
        nstring result(MAX_PATH, L'\0');

        result.resize(GetSystemDirectoryW(result.data(), result.size()));
        result.shrink_to_fit();

        return to_utf8_string(result);
    }

    abi::string get_process_path() {
        return get_module_path(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, nullptr);
    }

    abi::string get_executing_path() {
        return get_module_path(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, &get_executing_path);
    }

    std::string get_executing_directory() {
        return from_u8string(std::filesystem::path{to_u8string(get_executing_path())}.parent_path().generic_u8string());
    }

    std::uint32_t get_session_id() {
        if (DWORD session_id{}; ProcessIdToSessionId(GetCurrentProcessId(), &session_id)) {
            return session_id;
        }

        return std::numeric_limits<DWORD>::max();
    }

    zwstring_view get_service_account_name(service_account_type type) {
        switch (type) {
        case service_account_type::local_service:
            return LR"(NT AUTHORITY\LocalService)";
        case service_account_type::network_service:
            return LR"(NT AUTHORITY\NetworkService)";
        default:
            return {};
        }
    }

    error_checking_handler make_service_error_checker(const service_config& config) {
        return [name = config.name, context = config.context](bool success, std::string_view message) {
            if (!success) {
                throw source_code_aware_runtime_error{
                    U8("Name"), name, U8("Path"), context, U8("Message"), message, U8("Internal"), get_last_error()};
            }
        };
    }

    std::vector<abi::string> parse_command_line(zwstring_view command_line) {
        std::vector<abi::string> result;

        if (std::int32_t argc{}; const local_ptr<wchar_t*> argv{CommandLineToArgvW(command_line.c_str(), &argc)}) {
            for (std::size_t i = 0; i < static_cast<std::size_t>(argc); i++) {
                result.emplace_back(to_utf8_string(argv[i]));
            }
        }

        return result;
    }

    abi::wstring make_command_line(std::span<const std::string> args) {
        static constexpr auto escape = [](std::string_view arg) {
            std::string result;

            result.reserve(arg.size());

            if (arg.find_first_of(filesystem_tokens::command_line_special_group) != std::string_view::npos) {
                result.push_back(filesystem_tokens::quotation_mark);
            }

            // Escapes quotation marks.
            auto escaped = arg | std::views::transform([](char ch) {
                return ch == filesystem_tokens::quotation_mark ? filesystem_tokens::escaped_quotation_mark
                                                               : std::string_view{&ch, 1U};
            }) | std::views::join
                         | std::views::common;

            result.append(escaped.begin(), escaped.end());

            if (!result.empty() && result.front() == filesystem_tokens::quotation_mark) {
                result.push_back(filesystem_tokens::quotation_mark);
            }

            return result;
        };

        auto list  = args | std::views::transform(escape) | std::views::transform(&to_native_string);
        auto joint = join_with(list, std::wstring_view{&filesystem_tokens::command_line_separator_wide, 1U});

        return {joint.begin(), joint.end()};
    }

    void allocate_console_and_redirect() {
        AllocConsole();

        FILE* fp{};

        freopen_s(&fp, U8("CONOUT$"), U8("w"), stdout);
        freopen_s(&fp, U8("CONOUT$"), U8("w"), stderr);
        freopen_s(&fp, U8("CONIN$"), U8("r"), stdin);

        std::setvbuf(stdout, nullptr, _IONBF, 0U);
        std::setvbuf(stderr, nullptr, _IONBF, 0U);
    }

    void add_dll_directories(std::span<const std::string> directories) {
        for (std::error_code code; auto&& item : directories | std::views::filter([&](const auto& inner) {
                 return std::filesystem::is_directory(to_u8string(inner), code);
             })) {
            static_cast<void>(AddDllDirectory(to_native_string(item).c_str()));
        }
    }
} // namespace essence::win
