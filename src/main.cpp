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

#include "config_setup.hpp"
#include "service_config.hpp"
#include "service_manager.hpp"
#include "service_process.hpp"
#include "service_worker.hpp"
#include "startup_info.hpp"
#include "util.hpp"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <locale>
#include <string_view>
#include <utility>

#include <essence/char8_t_remediation.hpp>
#include <essence/cli/arg_parser.hpp>
#include <essence/cli/option.hpp>
#include <essence/compat.hpp>
#include <essence/encoding.hpp>
#include <essence/error_extensions.hpp>

#include <spdlog/spdlog.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI

#include <Windows.h>
#include <objbase.h>

using namespace essence;
using namespace essence::cli;
using namespace essence::win;

BOOL WINAPI DllMain(HMODULE module, DWORD reason, void* reserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        {
            if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
                OutputDebugStringW(L"CoInitializeEx failed.");

                return FALSE;
            }

            allocate_console_and_redirect();
            std::locale::global(std::locale{"en_US.UTF-8"});
            std::system(U8("chcp 65001"));
            SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_USER_DIRS);
            break;
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        [[fallthrough]];
    default:
        break;
    }

    return TRUE;
}

extern "C" {
ES_API(SVCHOSTIFY)
void WINAPI invokeW(HWND window, HINSTANCE instance, const wchar_t* command_line, std::int32_t show) try {
    std::atexit([] { std::system(U8("pause")); });

    const auto args = parse_command_line(command_line);

    auto opt_install = option<bool>{}
                           .set_bound_name(U8("install"))
                           .set_description(U8("Installs the service."))
                           .add_aliases(U8("i"))
                           .as_abstract();

    auto opt_uninstall = option<bool>{}
                             .set_bound_name(U8("uninstall"))
                             .set_description(U8("Uninstalls the service."))
                             .add_aliases(U8("u"))
                             .as_abstract();

    auto opt_config_file = option<std::string>{}
                               .set_bound_name(U8("config_file"))
                               .set_description(U8("Sets the configuration file path."))
                               .add_aliases(U8("c"))
                               .as_abstract();

    opt_config_file.on_validation([](std::string_view value, validation_result& result) {
        if (std::error_code code; !std::filesystem::is_regular_file(to_u8string(value), code)) {
            result.success = false;
            result.error   = U8("The configuration file path must be a regular file.");
        }
    });

    const arg_parser parser;

    parser.add_option(std::move(opt_install));
    parser.add_option(std::move(opt_uninstall));
    parser.add_option(std::move(opt_config_file));

    parser.on_output([](std::string_view message) {
        std::fwrite(message.data(), sizeof(char), message.size(), stdout);
        std::fputc(U8('\n'), stdout);
    });

    parser.on_error([](std::string_view message) { spdlog::error(message); });

    if (parser.parse(args); !parser) {
        return;
    }

    if (const auto info = parser.to_model<startup_info>()) {
        const auto make_config = [&] { return load_config_and_setup(info->config_file); };

        if (info->install) {
            auto config = make_config();
            {
                // Validates the worker initialization which gives concrete errors before installation.
                static_cast<void>(make_service_worker(config));
            }
            service_manager{std::move(config)}.install();

            return spdlog::info(U8("Service successfully installed."));
        }

        if (info->uninstall) {
            service_manager{make_config()}.uninstall();

            return spdlog::info(U8("Service successfully uninstalled."));
        }
    }
} catch (const std::exception& ex) {
    spdlog::error(ex.what());
}

ES_API(SVCHOSTIFY) void WINAPI ServiceMain(DWORD argc, wchar_t** argv) {
    const auto token = get_logger_shutdown_token();

    static_cast<void>(token);

    try {
        if (get_session_id() != 0) {
            throw source_code_aware_runtime_error{U8("The program can only be running in service mode.")};
        }

        const auto service_name = argv[0];

        service_process::instance().init(service_name);
        service_process::instance().run(make_service_worker_from_registry(service_name));
    } catch (const std::exception& ex) {
        spdlog::error(ex.what());
        OutputDebugStringW(to_native_string(ex.what()).c_str());
        service_process::instance().report_stopped();
    }
}
}
