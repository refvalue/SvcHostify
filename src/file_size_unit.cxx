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

module refvalue.svchostify:file_size_unit;
import essence.basic;
import std;

namespace essence {
    namespace {
        constexpr std::array unit_ratios{
            std::pair{std::string_view{U8("")}, 1ULL},
            std::pair{std::string_view{U8("KiB")}, 1024ULL},
            std::pair{std::string_view{U8("MiB")}, 1024 * 1024ULL},
            std::pair{std::string_view{U8("GiB")}, 1024 * 1024 * 1024ULL},
            std::pair{std::string_view{U8("TiB")}, 1024 * 1024 * 1024 * 1024ULL},
        };

        const auto unit_ratio_mapping = [] {
            return []<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::unordered_map<std::string_view, std::uint64_t, icase_string_hash, std::equal_to<>>{
                    unit_ratios[Is]...,
                };
            }(std::make_index_sequence<unit_ratios.size()>{});
        }();

        const auto unit_size_pattern = [] {
            const auto pattern =
                join_with(unit_ratios | std::views::drop(1) | std::views::keys, std::string_view{U8("|")})
                | std::ranges::to<std::string>();

            return format(U8(R"((\d+)\s*({})?)"), pattern);
        }();
    } // namespace

    std::optional<std::uint64_t> parse_file_size(std::string_view size) {
        static const std::regex pattern{unit_size_pattern,
            std::regex_constants::icase | std::regex_constants::ECMAScript | std::regex_constants::optimize};

        if (std::cmatch matches; std::regex_search(size.data(), size.data() + size.size(), matches, pattern)) {
            if (const auto iter = unit_ratio_mapping.find(matches[2].str()); iter != unit_ratio_mapping.end()) {
                return *from_string<std::uint64_t>(matches[1].str()) * iter->second;
            }
        }

        return std::nullopt;
    }

    std::string truncate_file_size_string(std::uint64_t size) {
        auto best_match = unit_ratios | std::views::reverse | std::views::transform([&](const auto& inner) {
            return std::pair{inner.first, size / inner.second};
        }) | std::views::drop_while([&](const auto& inner) { return inner.second != 0; });

        std::string result;

        if (best_match.empty()) {
            result.append(to_string(size));
        } else {
            result.append(to_string(best_match.front().second));
            result.push_back(U8(' '));
            result.append(best_match.front().first);
        }

        return result;
    }
} // namespace essence
