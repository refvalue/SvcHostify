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

#include "registry.hpp"

#include "filesystem_tokens.hpp"
#include "util.hpp"

#include <array>
#include <functional>
#include <ranges>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <essence/char8_t_remediation.hpp>
#include <essence/encoding.hpp>
#include <essence/error_extensions.hpp>
#include <essence/range.hpp>
#include <essence/string.hpp>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI

#include <Windows.h>
#include <shellapi.h>

namespace essence::win {
    namespace {
        const std::unordered_map<std::string_view, HKEY, icase_string_hash, std::equal_to<>> predefined_hkeys{
            {U8("HKEY_LOCAL_MACHINE"), HKEY_CLASSES_ROOT},
            {U8("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG},
            {U8("HKEY_CURRENT_USER"), HKEY_CURRENT_USER},
            {U8("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE},
            {U8("HKEY_USERS"), HKEY_USERS},

            {U8("HKCR"), HKEY_CLASSES_ROOT},
            {U8("HKCC"), HKEY_CURRENT_CONFIG},
            {U8("HKCU"), HKEY_CURRENT_USER},
            {U8("HKLM"), HKEY_LOCAL_MACHINE},
            {U8("HKU"), HKEY_USERS},
        };

        template <typename T>
            requires(std::same_as<T, std::uint32_t> || std::same_as<T, std::uint64_t>)
        struct registry_integer_adapter {
            using value_type = T;

            T value{};

            [[nodiscard]] T* data() noexcept {
                return &value;
            }

            [[nodiscard]] const T* data() const noexcept {
                return &value;
            }

            [[nodiscard]] static std::size_t size() noexcept {
                return sizeof(T);
            }

            static void resize(std::size_t size) noexcept {}
        };

        template <typename... Args>
        void check_registry_error(std::uint32_t code, Args&&... args) {
            if (code != ERROR_SUCCESS) {
                throw source_code_aware_runtime_error{
                    std::forward<Args>(args)..., U8("Internal"), get_system_error(code)};
            }
        }

        std::pair<HKEY, abi::wstring> decompose_registry_path(std::string_view path) {
            thread_local std::string preferred_path;
            auto preferred = path | std::views::transform([](char ch) {
                return ch == filesystem_tokens::generic_separator ? filesystem_tokens::preferred_separator : ch;
            });

            preferred_path.assign(preferred.begin(), preferred.end());

            const auto pure_path = trim_right(preferred_path, filesystem_tokens::preferred_separator_group);
            const auto first_component =
                pure_path.substr(0, preferred_path.find_first_of(filesystem_tokens::preferred_separator));

            if (const auto iter = predefined_hkeys.find(first_component);
                iter != predefined_hkeys.end() && first_component.size() < pure_path.size()) {
                return {iter->second, to_native_string(trim(first_component.data() + first_component.size() + 1,
                                          filesystem_tokens::preferred_separator_group))};
            }

            throw source_code_aware_runtime_error{U8("Key"), path, U8("Message"), U8("Illegal registry key.")};
        }

        void set_registry(
            std::string_view path, std::string_view name, std::uint32_t type, const void* value, std::size_t size) {
            auto&& [key, sub_key] = decompose_registry_path(path);

            check_registry_error(RegSetKeyValueW(key, sub_key.c_str(), to_native_string(name).c_str(), type, value,
                                     static_cast<DWORD>(size)),
                U8("Key"), path, U8("Name"), name, U8("Message"), U8("Failed to set the registry value."));
        }

        template <typename Container>
            requires std::is_standard_layout_v<typename Container::value_type>
        Container get_registry(std::string_view path, std::string_view name, std::uint32_t flags) {
            auto&& [key, sub_key] = decompose_registry_path(path);
            const auto wide_name  = to_native_string(name);

            DWORD size{};

            check_registry_error(RegGetValueW(key, sub_key.c_str(), wide_name.c_str(), flags, nullptr, nullptr, &size),
                U8("Key"), path, U8("Name"), name, U8("Message"),
                U8("Failed to get the storage size of the registry value."));

            Container result;

            result.resize(size / sizeof(typename Container::value_type));
            size = static_cast<DWORD>(result.size() * sizeof(typename Container::value_type));

            check_registry_error(
                RegGetValueW(key, sub_key.c_str(), wide_name.c_str(), flags, nullptr, result.data(), &size), U8("Key"),
                path, U8("Name"), name, U8("Message"), U8("Failed to get the context of the registry value."));

            return result;
        }
    } // namespace

    abi::string get_registry_string(std::string_view path, std::string_view name) {
        // Uses c_str() to automatically remove the trailing null terminator.
        return to_utf8_string(get_registry<std::wstring>(path, name, RRF_RT_REG_SZ).c_str()); // NOLINT(*-redundant-string-cstr)
    }

    std::vector<abi::string> get_registry_multi_string(std::string_view path, std::string_view name) {
        const auto buffer = get_registry<std::wstring>(path, name, RRF_RT_REG_MULTI_SZ);
        auto lines        = buffer | std::views::take(buffer.size() - 1) | std::views::split(L'\0')
                   | std::views::transform(
                       [](const auto& inner) { return to_utf8_string(std::wstring_view{inner.begin(), inner.end()}); });

        return {lines.begin(), lines.end()};
    }

    std::vector<std::byte> get_registry_binary(std::string_view path, std::string_view name) {
        return get_registry<std::vector<std::byte>>(path, name, RRF_RT_REG_BINARY);
    }

    std::uint32_t get_registry_dword(std::string_view path, std::string_view name) {
        return get_registry<registry_integer_adapter<std::uint32_t>>(path, name, RRF_RT_REG_DWORD).value;
    }

    std::uint64_t get_registry_qword(std::string_view path, std::string_view name) {
        return get_registry<registry_integer_adapter<std::uint64_t>>(path, name, RRF_RT_REG_QWORD).value;
    }

    void set_registry(std::string_view path, std::string_view name, std::span<const std::string> values) {
        auto joint = join_with(values | std::views::transform(&to_native_string), std::array{L'\0'});
        std::wstring multi_sz{joint.begin(), joint.end()};

        multi_sz.append(2, L'\0');

        set_registry(path, name, REG_MULTI_SZ, multi_sz.c_str(), multi_sz.size() * sizeof(wchar_t));
    }

    void set_registry(std::string_view path, std::string_view name, std::span<const std::byte> values) {
        set_registry(path, name, REG_BINARY, values.data(), values.size());
    }

    void set_registry(std::string_view path, std::string_view name, zstring_view value, bool expand_sz) {
        const auto native = to_native_string(value);

        set_registry(path, name, expand_sz ? REG_EXPAND_SZ : REG_SZ, native.c_str(), native.size() * sizeof(wchar_t));
    }

    void set_registry(std::string_view path, std::string_view name, std::uint32_t value) {
        set_registry(path, name, REG_DWORD, &value, sizeof(value));
    }

    void set_registry(std::string_view path, std::string_view name, std::uint64_t value) {
        set_registry(path, name, REG_QWORD, &value, sizeof(value));
    }

    void delete_registry(std::string_view path) {
        auto&& [key, sub_key] = decompose_registry_path(path);

        check_registry_error(RegDeleteTreeW(key, sub_key.c_str()), U8("Key"), path, U8("Message"),
            U8("Failed to delete the registry tree."));
    }

    void delete_registry(std::string_view path, std::string_view name) {
        auto&& [key, sub_key] = decompose_registry_path(path);

        check_registry_error(RegDeleteKeyValueW(key, sub_key.c_str(), to_native_string(name).c_str()), U8("Key"), path,
            U8("Name"), name, U8("Message"), U8("Failed to delete the registry value."));
    }
} // namespace essence::win
