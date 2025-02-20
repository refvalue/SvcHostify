export module refvalue.svchostify:config_setup;
import :service_config;
import std;

export namespace essence::win {
    void setup_config(const service_config& config, bool enable_file_logging = false);
    service_config load_config_and_setup(std::string_view path, bool enable_file_logging = false);
    std::shared_ptr<void> get_logger_shutdown_token();
} // namespace essence::win
