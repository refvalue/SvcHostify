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

#include "file_size_unit.hpp"
#include "util.hpp"

#include <atomic>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <utility>

#include <essence/char8_t_remediation.hpp>
#include <essence/error_extensions.hpp>
#include <essence/io/fs_operator.hpp>
#include <essence/io/stdio_watcher.hpp>
#include <essence/json_compat.hpp>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

using namespace essence::io;

namespace essence::win {
    namespace {
        const service_config::logger_config default_logger_config{
            .base_path = U8("log/svchostify.log"),
            .max_size  = U8("50 MiB"),
            .max_files = 5U,
        };

        constexpr std::pair valid_file_size_range{1024ULL, 1024 * 1024 * 1024 * 2ULL};
        constexpr std::pair valid_file_count_range{1ULL, 32ULL};

        class stdio_to_sink_dispatcher {
            struct formatter : spdlog::formatter {
                void format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest) override {
                    dest.assign(msg.payload);
                }

                [[nodiscard]] std::unique_ptr<spdlog::formatter> clone() const override {
                    return std::make_unique<formatter>(*this);
                }
            };

        public:
            stdio_to_sink_dispatcher()
                : stdout_watcher_{stdio_watcher_mode::output}, stderr_watcher_{stdio_watcher_mode::error} {
                stdout_watcher_.on_message(std::bind_front(&stdio_to_sink_dispatcher::process_message, this));
                stderr_watcher_.on_message(std::bind_front(&stdio_to_sink_dispatcher::process_message, this));
                stdout_watcher_.start();
                stderr_watcher_.start();
            }

            void set_sink(spdlog::sink_ptr sink) noexcept {
                sink->set_formatter(std::make_unique<formatter>());
                sink->set_level(spdlog::level::info);
                sink_.store(std::move(sink), std::memory_order::release);
            }

        private:
            void process_message(std::string_view message) const {
                if (const auto sink = sink_.load(std::memory_order::acquire)) {
                    sink->log(spdlog::details::log_msg{U8(""), spdlog::level::info, message});
                }
            }

            std::atomic<spdlog::sink_ptr> sink_;
            stdio_watcher stdout_watcher_;
            stdio_watcher stderr_watcher_;
        };

        std::atomic<std::shared_ptr<stdio_to_sink_dispatcher>> dispatcher;

        auto parse_logger_config(const service_config& config) {
            const auto logger_config = config.logger.value_or(default_logger_config);

            if (logger_config.base_path.empty()) {
                throw source_code_aware_runtime_error{U8("The logger base path must be non-empty.")};
            }

            // Creates the logging directory if the base path contains a parent path.
            if (const std::filesystem::path base_path{to_u8string(logger_config.base_path)};
                base_path.has_parent_path()) {
                const auto logging_directory = base_path.parent_path();

                if (std::error_code code; !std::filesystem::create_directories(logging_directory, code) && code) {
                    throw source_code_aware_runtime_error{U8("Logging Directory"),
                        from_u8string(logging_directory.generic_u8string()), U8("Message"),
                        U8("Failed to create the logging directory."), U8("Internal"), code.message()};
                }
            }

            const auto max_size = parse_file_size(logger_config.max_size.value_or(*default_logger_config.max_size));

            if (!max_size) {
                throw source_code_aware_runtime_error{
                    U8("Max Size"), *logger_config.max_size, U8("Message"), U8("Invalid max file size of the logger.")};
            }

            if (auto&& [min, max] = valid_file_size_range; *max_size < min || *max_size > max) {
                throw source_code_aware_runtime_error{U8("Max Size"), *logger_config.max_size, U8("Lower Bound"),
                    truncate_file_size_string(min), U8("Upper Bound"), truncate_file_size_string(max), U8("Message"),
                    U8("The max file size was out of range.")};
            }

            const auto max_files = logger_config.max_files.value_or(*default_logger_config.max_files);

            if (auto&& [min, max] = valid_file_count_range; max_files < min || max_files > max) {
                throw source_code_aware_runtime_error{U8("Max Files"), *logger_config.max_files, U8("Lower Bound"), min,
                    U8("Upper Bound"), max, U8("Message"), U8("The max file count was out of range.")};
            }

            struct context {
                enum class json_serialization {
                    camel_case,
                };

                std::string base_path;
                std::size_t max_size{};
                std::size_t max_files{};
            };

            return context{logger_config.base_path, *max_size, max_files};
        }

        void setup_logger(const service_config& config, bool enable_file_logging) {
            const auto logger_config = parse_logger_config(config);

            if (enable_file_logging) {
                auto instance = std::make_shared<stdio_to_sink_dispatcher>();

                instance->set_sink(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    logger_config.base_path, logger_config.max_size, logger_config.max_files));
                dispatcher.store(std::move(instance), std::memory_order::release);
            } else {
                dispatcher.store(nullptr, std::memory_order::release);
            }

            auto sinks = std::views::single(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

            spdlog::set_default_logger(
                std::make_shared<spdlog::logger>(U8("service-logger"), sinks.begin(), sinks.end()));

            spdlog::default_logger()->flush_on(spdlog::level::info);
            spdlog::info(U8("Logger configuration: {}"), json(logger_config).dump(4));
        }
    } // namespace

    void setup_config(const service_config& config, bool enable_file_logging = false) {
        static const auto executing_directory = get_executing_directory();
        const auto working_directory          = config.working_directory.value_or(executing_directory);

        spdlog::info(U8("Working directory: {}"), working_directory);

        if (std::error_code code; (std::filesystem::current_path(to_u8string(working_directory), code), code)) {
            throw source_code_aware_runtime_error{U8("Working Directory"), working_directory, U8("Message"),
                U8("Failed to set the working directory."), U8("Internal"), code.message()};
        }

        setup_logger(config, enable_file_logging);
        spdlog::info(json(config).dump(4));

        auto dll_directories = config.dll_directories.value_or(std::vector<std::string>{});

        dll_directories.emplace_back(executing_directory);
        add_dll_directories(dll_directories);
    }

    service_config load_config_and_setup(std::string_view path, bool enable_file_logging) {
        auto config = [&] {
            try {
                return json::parse(*io::get_native_fs_operator().open_read(path)).get<service_config>();
            } catch (const std::exception& ex) {
                throw source_code_aware_runtime_error{
                    U8("Message"), U8("Failed to parse the configuration file."), U8("Internal"), ex.what()};
            }
        }();

        setup_config(config, enable_file_logging);

        return config;
    }

    std::shared_ptr<void> get_logger_shutdown_token() {
        return {&dispatcher, [](auto) {
                    spdlog::default_logger()->flush();
                    dispatcher.store(nullptr, std::memory_order::release);
                }};
    }
} // namespace essence::win
