export module refvalue.svchostify:util;
import :common_types;
import essence.basic;
import std;

export namespace essence::win {
    abi::string get_system_error(std::uint32_t code);
    abi::string get_last_error();
    abi::string get_system_directory();
    abi::string get_process_path();
    abi::string get_executing_path();
    std::string get_executing_directory();
    std::uint32_t get_session_id();
    zwstring_view get_service_account_name(service_account_type type);
    std::vector<abi::string> parse_command_line(zwstring_view command_line);
    abi::wstring make_command_line(std::span<const std::string> args);
    void allocate_console_and_redirect();
    void add_dll_directories(std::span<const std::string> directories);
} // namespace essence::win
