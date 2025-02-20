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

module refvalue.svchostify:filesystem_tokens;
import essence.basic;
import std;

namespace essence::win {
    struct filesystem_tokens {
        static constexpr char generic_separator              = U8('/');
        static constexpr char preferred_separator            = U8('\\');
        static constexpr char command_line_separator         = U8(' ');
        static constexpr char quotation_mark                 = U8('\"');
        static constexpr wchar_t command_line_separator_wide = L' ';

        static constexpr std::string_view escaped_quotation_mark{U8("\\\"")};
        static constexpr std::string_view command_line_special_group{U8(" \"")};
        static constexpr std::string_view preferred_separator_group{&preferred_separator, 1U};
    };
} // namespace essence::win
