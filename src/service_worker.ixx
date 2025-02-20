export module refvalue.svchostify:service_worker;
import :abstract.service_worker;
import :service_config;
import essence.basic;
import std;

export namespace essence::win {
    abstract::service_worker make_service_worker(service_config config);
    abstract::service_worker make_service_worker_from_registry(zwstring_view service_name);
} // namespace essence::win
