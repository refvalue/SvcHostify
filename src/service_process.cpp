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

#include "service_process.hpp"

#include "config_setup.hpp"
#include "win32/svchost.h"

#include <exception>
#include <filesystem>
#include <future>
#include <optional>
#include <thread>
#include <utility>

#include <essence/abi/string.hpp>
#include <essence/char8_t_remediation.hpp>
#include <essence/encoding.hpp>
#include <essence/error_extensions.hpp>
#include <essence/exception.hpp>

#include <spdlog/spdlog.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI


#include <Windows.h>

namespace essence::win {
    namespace {
        constexpr DWORD pending_wait_hint = 10000;
    }

    class service_process::impl {
    public:
        impl() : status_{.dwServiceType = SERVICE_WIN32_SHARE_PROCESS}, status_handle_{}, global_data_{} {}

        void init(zwstring_view service_name) {
            service_name_.assign(service_name);
            status_handle_ = RegisterServiceCtrlHandlerExW(service_name_.c_str(), &impl::service_ctrl_handler, this);
        }

        void run(abstract::service_worker worker) {
            worker_.emplace(std::move(worker));
            start();
        }

        void report_stopped() {
            report_status(SERVICE_STOPPED);
            spdlog::info("The service has stopped.");
        }

        void set_global_data(const void* data) noexcept {
            global_data_ = static_cast<const SVCHOST_GLOBAL_DATA*>(data);
        }

    private:
        void run_business() try {
            std::promise<void> promise;
            std::jthread worker{[&] {
                try {
                    worker_->run();
                    promise.set_value();
                } catch (const std::exception&) {
                    promise.set_exception(std::current_exception());
                }
            }};

            // The cookie is a facade and does not need to be closed.
            if (HANDLE cookie{}; global_data_ && global_data_->RegisterStopCallback) {
                global_data_->RegisterStopCallback(
                    &cookie, service_name_.c_str(), worker.native_handle(),
                    [](void* context, BOOLEAN timeout) { static_cast<impl*>(context)->report_stopped(); }, this,
                    WT_EXECUTEONLYONCE);
            }

            promise.get_future().get();
        } catch (const std::exception&) {
            aggregate_error::throw_nested(
                source_code_aware_runtime_error{U8("An error occurred during the service running.")});
        }

        void start() {
            report_status(SERVICE_START_PENDING, pending_wait_hint);
            spdlog::info("The service start is pending.");

            try {
                worker_->on_start();
            } catch (const std::exception&) {
                aggregate_error::throw_nested(source_code_aware_runtime_error{U8("Failed to start the service.")});
            }

            report_status(SERVICE_RUNNING);
            spdlog::info("The service is running.");
            run_business();
            report_stopped();
        }

        void stop() {
            report_status(SERVICE_STOP_PENDING, pending_wait_hint);
            spdlog::info("The service stop is pending.");

            try {
                worker_->on_stop();
            } catch (...) {
            }
        }

        static DWORD WINAPI service_ctrl_handler(DWORD control, DWORD event_type, void* event_data, void* context) {
            switch (control) {
            case SERVICE_CONTROL_STOP:
                static_cast<impl*>(context)->stop();
                break;
            case SERVICE_CONTROL_INTERROGATE:
                [[fallthrough]];
            default:
                break;
            }

            return NO_ERROR;
        }

        void report_status(DWORD current_state, DWORD wait_hint = {}) {
            static DWORD check_point{};

            status_.dwCurrentState     = current_state;
            status_.dwWaitHint         = wait_hint;
            status_.dwControlsAccepted = current_state == SERVICE_START_PENDING ? 0U : SERVICE_ACCEPT_STOP;
            status_.dwCheckPoint =
                (current_state == SERVICE_RUNNING || current_state == SERVICE_STOPPED) ? 0U : ++check_point;

            static_cast<void>(SetServiceStatus(status_handle_, &status_));
        }

        SERVICE_STATUS status_;
        SERVICE_STATUS_HANDLE status_handle_;
        std::optional<abstract::service_worker> worker_;
        abi::wstring service_name_;
        const SVCHOST_GLOBAL_DATA* global_data_;
    };

    service_process::~service_process() = default;

    const service_process& service_process::instance() {
        static const service_process instance;

        return instance;
    }

    void service_process::init(zwstring_view service_name) const {
        impl_->init(service_name);
    }

    void service_process::run(abstract::service_worker worker) const {
        impl_->run(std::move(worker));
    }

    void service_process::report_stopped() const {
        impl_->report_stopped();
    }

    void service_process::set_global_data(const void* data) const noexcept {
        impl_->set_global_data(data);
    }

    service_process::service_process() : impl_{std::make_unique<impl>()} {}
} // namespace essence::win

// ReSharper disable once CppParameterMayBeConstPtrOrRef
extern "C" ES_API(SVCHOSTIFY) void WINAPI SvchostPushServiceGlobals(SVCHOST_GLOBAL_DATA* global_data) {
    essence::win::service_process::instance().set_global_data(global_data);
}
