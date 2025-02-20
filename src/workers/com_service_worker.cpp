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

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#include <Windows.h>
#include <comdef.h>
#include <comutil.h>

module refvalue.svchostify;
import :abstract.service_worker;
import :service_config;
import :service_worker;
import :win32.ISvcHostify;
import essence.basic;
import std;

namespace essence::win {
    namespace {
        using safe_array_ptr =
            std::unique_ptr<SAFEARRAY, decltype([](SAFEARRAY* inner) { static_cast<void>(SafeArrayDestroy(inner)); })>;

        template <typename... Args>
        void check_com_error(HRESULT hr, Args&&... args) {
            try {
                _com_util::CheckError(hr);
            } catch (const _com_error& ex) {
                throw formatted_runtime_error{
                    std::forward<Args>(args)..., U8("Internal"), to_utf8_string(ex.ErrorMessage())};
            }
        }

        safe_array_ptr make_args(std::span<const std::string> args) {
            SAFEARRAYBOUND bound{
                .cElements = static_cast<ULONG>(args.size()),
            };

            safe_array_ptr array{SafeArrayCreate(VT_BSTR, 1, &bound)};
            BSTR* ptr{};

            check_com_error(SafeArrayAccessData(array.get(), reinterpret_cast<void**>(&ptr)), U8("Message"),
                U8("Failed to access the array data."));

            const scope_exit data_scope{[&] {
                check_com_error(
                    SafeArrayUnaccessData(array.get()), U8("Message"), U8("Failed to unaccess the array data."));
            }};

            auto bstr_args = args | std::views::transform([](const auto& inner) {
                return _bstr_t{to_native_string(inner).c_str()}.Detach();
            });

            std::ranges::copy(bstr_args, ptr);

            return array;
        }

        class com_service_worker {
        public:
            explicit com_service_worker(service_config config) : config_{std::move(config)} {
                if (config_.context.empty()) {
                    throw formatted_runtime_error{
                        U8("The context must be a non-empty CLSID of your ISvcHostify implementation coclass.")};
                }

                check_com_error(broker_.CreateInstance(to_native_string(config_.context).c_str()), U8("CLSID"),
                    config_.context, U8("Message"), U8("Failed to create an instance."));
            }

            com_service_worker(com_service_worker&&) noexcept = default;

            com_service_worker& operator=(com_service_worker&&) noexcept = default;

            [[nodiscard]] [[maybe_unused]] const service_config& config() const noexcept {
                return config_;
            }

            [[maybe_unused]] static void on_start() noexcept {}

            void on_stop() const {
                check_com_error(broker_->OnStop(), U8("CLSID"), config_.context, U8("Message"),
                    U8("An error orrcurred inside the ISvcHostify instance when stopping."));
            }

            [[maybe_unused]] void run() const {
                check_com_error(broker_->Run(make_args(config_.arguments.value_or(std::vector<std::string>{})).get()),
                    U8("CLSID"), config_.context, U8("Message"),
                    U8("An error orrcurred inside the ISvcHostify instance when running."));
            }

        private:
            service_config config_;
            ISvcHostifyPtr broker_;
        };
    } // namespace

    abstract::service_worker make_com_service_worker(service_config config) {
        return abstract::service_worker{com_service_worker{std::move(config)}};
    }
} // namespace essence::win
