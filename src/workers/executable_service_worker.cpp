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

module refvalue.svchostify;
import :abstract.service_worker;
import :service_config;
import :service_worker;
import :util;
import essence.basic;
import std;

namespace essence::win {
    namespace {
        using kernel_handle = unique_handle<&CloseHandle>;

        class executable_service_worker {
        public:
            explicit executable_service_worker(service_config config) : config_{std::move(config)} {
                if (config_.context.empty()) {
                    throw formatted_runtime_error{U8("The context must be a non-empty executable path.")};
                }

                if (std::error_code code; !std::filesystem::is_regular_file(to_u8string(config_.context), code)) {
                    throw formatted_runtime_error{U8("Executable Path"), config_.context, U8("Message"),
                        U8("The executable path must be a regular file.")};
                }
            }

            executable_service_worker(executable_service_worker&&) noexcept = default;

            ~executable_service_worker() {
                on_stop();
            }

            executable_service_worker& operator=(executable_service_worker&&) noexcept = default;

            [[nodiscard]] [[maybe_unused]] const service_config& config() const noexcept {
                return config_;
            }

            [[maybe_unused]] void on_start() {
                STARTUPINFOW si{.cb = sizeof(STARTUPINFOW)};
                PROCESS_INFORMATION pi{};

                if (CreateProcessW(to_native_string(config_.context).c_str(),
                        config_.arguments ? make_command_line(*config_.arguments).data() : nullptr, nullptr, nullptr,
                        FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) {
                    wrapped_thread_.reset(pi.hThread);
                    wrapped_process_.reset(pi.hProcess);
                } else {
                    throw formatted_runtime_error{U8("Failed to create the process.")};
                }
            }

            void on_stop() const {
                if (wrapped_process_) {
                    if (config_.post_quit_message.value_or(false)) {
                        PostThreadMessageW(GetThreadId(wrapped_thread_.get()), WM_QUIT, 0U, 0);
                    } else {
                        TerminateProcess(wrapped_process_.get(), 0U);
                    }
                }
            }

            [[maybe_unused]] void run() {
                if (wrapped_process_) {
                    WaitForSingleObject(wrapped_process_.get(), INFINITE);
                }

                wrapped_thread_.reset();
                wrapped_process_.reset();
            }

        private:
            service_config config_;
            kernel_handle wrapped_thread_;
            kernel_handle wrapped_process_;
        };
    } // namespace

    abstract::service_worker make_executable_service_worker(service_config config) {
        return abstract::service_worker{executable_service_worker{std::move(config)}};
    }
} // namespace essence::win
