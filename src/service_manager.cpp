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

#include "service_manager.hpp"

#include "common_types.hpp"
#include "filesystem_tokens.hpp"
#include "registry.hpp"
#include "service_registry_keys.hpp"
#include "util.hpp"

#include <array>
#include <filesystem>
#include <functional>
#include <string_view>
#include <utility>

#include <essence/abi/string.hpp>
#include <essence/char8_t_remediation.hpp>
#include <essence/crypto/digest.hpp>
#include <essence/encoding.hpp>
#include <essence/error_extensions.hpp>
#include <essence/format_remediation.hpp>
#include <essence/managed_handle.hpp>

#include <spdlog/spdlog.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI

#include <Windows.h>

using namespace essence::crypto;

namespace essence::win {
    namespace {
        using sc_handle = unique_handle<&CloseServiceHandle>;

        const auto svchost_executable = std::filesystem::path{to_u8string(get_system_directory())} / u8"svchost.exe";
    } // namespace

    class service_manager::impl {
    public:
        explicit impl(service_config config)
            : config_{std::move(config)}, checker_{make_service_error_checker(config_)},
              service_name_{to_native_string(config_.name)},
              group_name_{format(U8("Broker_{}_{}"), config_.name, make_digest(digest_mode::sha3_224, config_.name))},
              group_key_{service_registry_keys::svchost_key},
              service_param_key_{format(service_registry_keys::service_param_key_pattern, config_.name)} {
            group_key_.push_back(filesystem_tokens::preferred_separator);
            group_key_.append(group_name_);
        }

        void install() const {
            const auto path = to_native_string(
                format(U8("{} -k {}"), from_u8string(svchost_executable.generic_u8string()), group_name_));

            const sc_handle handle{CreateServiceW(ensure_scm().get(), service_name_.c_str(),
                to_native_string(config_.display_name).c_str(), SERVICE_ALL_ACCESS, SERVICE_WIN32_SHARE_PROCESS,
                SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, path.c_str(), nullptr, nullptr, nullptr,
                get_service_account_name(config_.account_type).c_str(), nullptr)};

            checker_(static_cast<bool>(handle), U8("Failed to install the service."));

            if (config_.description) {
                auto description = to_native_string(*config_.description);
                SERVICE_DESCRIPTIONW service_desc{.lpDescription = description.data()};

                checker_(ChangeServiceConfig2W(handle.get(), SERVICE_CONFIG_DESCRIPTION, &service_desc),
                    U8("Failed to set the description of the service."));
            }

            register_svchost();
        }

        void uninstall() const {
            SERVICE_STATUS status{};
            const sc_handle handle{open_service(SERVICE_STOP | DELETE)};

            checker_(ControlService(handle.get(), SERVICE_CONTROL_STOP, &status)
                         || GetLastError() == ERROR_SERVICE_NOT_ACTIVE,
                U8("Failed to stop the service."));
            checker_(DeleteService(handle.get()), U8("Failed to uninstall the service."));

            unregister_svchost();
        }

        [[nodiscard]] bool installed() const {
            return static_cast<bool>(open_service<false>(SERVICE_QUERY_CONFIG));
        }

    private:
        [[nodiscard]] sc_handle ensure_scm() const {
            sc_handle handle{OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS)};

            checker_(static_cast<bool>(handle), U8("Failed open the Service Control Manager."));

            return handle;
        }

        template <bool Raise = true>
        [[nodiscard]] sc_handle open_service(DWORD desired_access) const {
            sc_handle handle{OpenServiceW(ensure_scm().get(), service_name_.c_str(), desired_access)};

            if constexpr (Raise) {
                checker_(static_cast<bool>(handle), U8("Failed to open service."));
            }

            return handle;
        }

        void register_svchost() const {
            // Enables COM initialization for svchost.exe.
            set_registry(service_registry_keys::svchost_key, group_name_, std::array{config_.name});
            set_registry(group_key_, service_registry_keys::co_initialize_security_param, 1U);

            // Sets service parameters.
            set_registry(service_param_key_, service_registry_keys::service_dll, get_executing_path(), true);
            set_registry(service_param_key_, service_registry_keys::service_dll_unload_on_stop, 1U);
            set_registry(service_param_key_, service_registry_keys::service_main, U8("ServiceMain"));
            set_registry(service_param_key_, service_registry_keys::startup_configuration, config_.to_msgpack_base64());
        }

        void unregister_svchost() const try {
            delete_registry(group_key_);
            delete_registry(service_registry_keys::svchost_key, group_name_);
        } catch (const std::exception& ex) {
            spdlog::warn(ex.what());
        }

        service_config config_;
        error_checking_handler checker_;
        abi::nstring service_name_;
        std::string group_name_;
        std::string group_key_;
        std::string service_param_key_;
    };

    service_manager::service_manager(service_config config) : impl_{std::make_unique<impl>(std::move(config))} {}

    service_manager::service_manager(service_manager&&) noexcept = default;

    service_manager::~service_manager() = default;

    service_manager& service_manager::operator=(service_manager&&) noexcept = default;

    void service_manager::install() const {
        impl_->install();
    }

    void service_manager::uninstall() const {
        impl_->uninstall();
    }

    bool service_manager::installed() const {
        return impl_->installed();
    }
} // namespace essence::win
