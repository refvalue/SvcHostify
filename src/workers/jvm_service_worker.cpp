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
#include <essence/compat.hpp>

#include <jni.h>

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
import essence.jni;
import std;

using namespace essence::jni;

namespace essence::win {
    namespace {
        using unique_module = unique_handle<&FreeLibrary>;

        std::atomic_int32_t jvm_class_key{1};

        class jvm_initializer {
        public:
            jvm_initializer(std::string_view jdk_directory, std::string_view class_path) : create_java_vm_{} {
                load_jvm(jdk_directory);

                std::string option_str{U8("-Djava.class.path=")};

                JavaVM* vm{};
                JNIEnv* env{};
                JavaVMOption option{
                    .optionString = option_str.append(class_path).data(),
                };

                JavaVMInitArgs args{
                    .version            = JNI_VERSION_1_6,
                    .nOptions           = 1,
                    .options            = &option,
                    .ignoreUnrecognized = JNI_FALSE,
                };

                if (create_java_vm_(&vm, reinterpret_cast<void**>(&env), &args) != JNI_OK) {
                    throw formatted_runtime_error{U8("Failed to create Java VM.")};
                }

                jvm::instance().init(vm);
            }

        private:
            void load_jvm(std::string_view jdk_directory) {
                const auto bin_directory = std::filesystem::path{to_u8string(jdk_directory)} / u8"bin";
                const auto jvm_path      = bin_directory / u8"server" / u8"jvm.dll";
                const auto awt_path      = bin_directory / u8"awt.dll";

                add_dll_directories(std::array{from_u8string(bin_directory.generic_u8string())});

                if (jvm_module_.reset(
                        LoadLibraryExW(jvm_path.generic_wstring().c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS));
                    !jvm_module_) {
                    throw formatted_runtime_error{U8("JVM Runtime"), from_u8string(jvm_path.generic_u8string()),
                        U8("Message"), U8("Failed to load JVM.")};
                }

                if (awt_module_.reset(
                        LoadLibraryExW(awt_path.generic_wstring().c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS));
                    !awt_module_) {
                    throw formatted_runtime_error{U8("AWT Runtime"), from_u8string(awt_path.generic_u8string()),
                        U8("Message"), U8("Failed to load AWT.")};
                }

                if (create_java_vm_ = reinterpret_cast<decltype(create_java_vm_)>(
                        GetProcAddress(jvm_module_.get(), U8("JNI_CreateJavaVM")));
                    create_java_vm_ == nullptr) {
                    throw formatted_runtime_error{U8("Failed to load the 'JNI_CreateJavaVM' function.")};
                }
            }

            unique_module jvm_module_;
            unique_module awt_module_;
            decltype(&JNI_CreateJavaVM) create_java_vm_;
        };

        void handle_java_exception() {
            if (const auto ex = try_catch_exception()) {
                throw formatted_runtime_error{
                    U8("Message"), U8("An exception was thrown inside the java code."), U8("Java Exception"), *ex};
            }
        }

        class jvm_service_worker {
        public:
            explicit jvm_service_worker(service_config config)
                : config_{std::move(config)}, method_run_{}, method_on_stop_{} {
                if (config_.context.empty()) {
                    throw formatted_runtime_error{U8("Class Path"), config_.context, U8("Message"),
                        U8("The context must be a non-empty CLASSPATH for JVM bootstrap.")};
                }

                if (!config_.jdk_directory) {
                    throw formatted_runtime_error{U8("The JDK directory must be set.")};
                }

                if (std::error_code code; !std::filesystem::is_directory(to_u8string(*config_.jdk_directory), code)) {
                    throw formatted_runtime_error{
                        U8("JDK Directory"), config_.context, U8("Message"), U8("The JDK directory must exist.")};
                }

                const auto class_key = jvm_class_key.fetch_add(1, std::memory_order::acq_rel);
                [[maybe_unused]] ES_KEEP_ALIVE static const jvm_initializer jvm_init{
                    *config_.jdk_directory,
                    config_.context,
                };

                // Makes Java method bindings.
                class_svchost_broker_ = reflector::instance().add_class(class_key, U8("org/refvalue/SvcHostify"));
                method_run_ =
                    reflector::instance().add_static_method(class_key, {1, U8("run"), U8("([Ljava/lang/String;)V")});

                method_on_stop_ = reflector::instance().add_static_method(class_key, {2, U8("onStop"), U8("()V")});
            }

            jvm_service_worker(jvm_service_worker&&) noexcept = default;

            jvm_service_worker& operator=(jvm_service_worker&&) noexcept = default;

            [[nodiscard]] const service_config& config() const noexcept {
                return config_;
            }

            [[maybe_unused]] static void on_start() noexcept {}

            [[maybe_unused]] void on_stop() const {
                jvm::instance().ensure_env()->CallStaticVoidMethod(class_svchost_broker_.get(), method_on_stop_);
                handle_java_exception();
            }

            [[maybe_unused]] void run() const {
                jvm::instance().ensure_env()->CallStaticVoidMethod(class_svchost_broker_.get(), method_run_,
                    scoped::make_array(config_.arguments.value_or(std::vector<std::string>{})).get());

                handle_java_exception();
            }

        private:
            service_config config_;
            global_ref_ex<jclass> class_svchost_broker_;
            jmethodID method_run_;
            jmethodID method_on_stop_;
        };
    } // namespace

    abstract::service_worker make_jvm_service_worker(service_config config) {
        return abstract::service_worker{jvm_service_worker{std::move(config)}};
    }
} // namespace essence::win
