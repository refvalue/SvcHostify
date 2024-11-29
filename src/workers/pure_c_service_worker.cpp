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

#include "../service_worker.hpp"

#include <cstddef>
#include <memory>
#include <new>
#include <span>
#include <string>
#include <utility>

#include <essence/char8_t_remediation.hpp>
#include <essence/encoding.hpp>
#include <essence/error_extensions.hpp>
#include <essence/managed_handle.hpp>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI

#include <Windows.h>

namespace essence::win {
    namespace {
        using unique_module = unique_handle<&FreeLibrary>;

        using refvalue_svchostify_run_ptr     = void (*)(std::size_t argc, const char* argv[]);
        using refvalue_svchostify_on_stop_ptr = void (*)();

        std::unique_ptr<const char*[]> make_argv(std::span<const std::string> args) {
            auto argv = std::make_unique_for_overwrite<const char*[]>(args.size());

            for (std::size_t i = 0; i < args.size(); i++) {
                argv[i] = args[i].c_str();
            }

            return argv;
        }

        class pure_c_service_worker {
        public:
            explicit pure_c_service_worker(service_config config) : config_{std::move(config)}, run_{}, on_stop_{} {
                if (config_.context.empty()) {
                    throw source_code_aware_runtime_error{U8("The context must be a non-empty DLL path.")};
                }

                if (module_dll_.reset(LoadLibraryExW(
                        to_native_string(config_.context).c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS));
                    !module_dll_) {
                    throw source_code_aware_runtime_error{
                        U8("DLL Path"), config_.context, U8("Message"), U8("Failed to load the DLL.")};
                }

                if (run_ = reinterpret_cast<refvalue_svchostify_run_ptr>(
                        GetProcAddress(module_dll_.get(), U8("refvalue_svchostify_run")));
                    run_ == nullptr) {
                    throw source_code_aware_runtime_error{U8("Failed to load the 'refvalue_svchostify_run' function.")};
                }

                if (on_stop_ = reinterpret_cast<refvalue_svchostify_on_stop_ptr>(
                        GetProcAddress(module_dll_.get(), U8("refvalue_svchostify_on_stop")));
                    on_stop_ == nullptr) {
                    throw source_code_aware_runtime_error{
                        U8("Failed to load the 'refvalue_svchostify_on_stop' function.")};
                }
            }

            pure_c_service_worker(pure_c_service_worker&&) noexcept = default;

            pure_c_service_worker& operator=(pure_c_service_worker&&) noexcept = default;

            [[nodiscard]] [[maybe_unused]] const service_config& config() const noexcept {
                return config_;
            }

            [[maybe_unused]] static void on_start() noexcept {}

            void on_stop() const {
                on_stop_();
            }

            [[maybe_unused]] void run() const {
                if (config_.arguments) {
                    run_(config_.arguments->size(),
                        make_argv(config_.arguments.value_or(std::vector<std::string>{})).get());
                } else {
                    std::uintptr_t dummy{};

                    run_(0, new (&dummy) const char*[0]);
                }
            }

        private:
            service_config config_;
            unique_module module_dll_;
            refvalue_svchostify_run_ptr run_;
            refvalue_svchostify_on_stop_ptr on_stop_;
        };
    } // namespace

    abstract::service_worker make_pure_c_service_worker(service_config config) {
        return abstract::service_worker{pure_c_service_worker{std::move(config)}};
    }
} // namespace essence::win
