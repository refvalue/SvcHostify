module;

#include <essence/char8_t_remediation.hpp>

module refvalue.svchostify;
import essence.crypto;
import essence.serialization;
import std;

namespace essence::win {
    service_config::logger_config service_config::default_values::logger_defaults::to_config() const {
        return {
            .base_path = base_path,
            .max_size  = max_size,
            .max_files = max_files,
        };
    }

    [[nodiscard]] const service_config::default_values& service_config::defaults() {
        static const default_values defaults{
            .standalone        = true,
            .post_quit_message = false,
            .working_directory = get_executing_directory(),
            .dll_directories   = {{get_executing_directory()}},
            .logger =
                {
                    .base_path = U8("logs/svchostify.log"),
                    .max_size  = U8("50 MiB"),
                    .max_files = 5U,
                },
        };

        return defaults;
    }

    service_config service_config::from_msgpack_base64(std::string_view base64) {
        return json::from_msgpack(crypto::base64_decode(base64)).get<service_config>();
    }

    abi::string service_config::to_msgpack_base64() const {
        return crypto::base64_encode(json::to_msgpack(*this));
    }

    error_checking_handler make_service_error_checker(const service_config& config) {
        return [name = config.name, context = config.context](bool success, std::string_view message) {
            if (!success) {
                throw formatted_runtime_error{
                    U8("Name"), name, U8("Path"), context, U8("Message"), message, U8("Internal"), get_last_error()};
            }
        };
    }
} // namespace essence::win
