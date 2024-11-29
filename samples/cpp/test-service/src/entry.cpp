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

#include <atomic>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <string_view>
#include <thread>

namespace {
    class test_service {
    public:
        test_service() : running_{false} {}

        /**
         * @brief The service entry point.
         * @param argc The count of the arguments.
         * @param argv The input arguments.
         */
        void run(std::size_t argc, const char* argv[]) {
            using std::chrono_literals::operator""ms;

            std::cout << "A Svchost run from Cplusplus.\n";
            std::cout << "All outputs to stdout will be redirected to the logging file that you configured.\n";
            std::cout << "Input arguments:\n";

            for (std::size_t i = 0; i < argc; i++) {
                std::cout << argv[i] << '\n';
            }

            static constexpr std::string_view file_name{"output_cplusplus.txt"};
            static constexpr std::string_view text{"It's good to write text to your own file for logging."};
            {
                std::ofstream stream{"output_cplusplus.txt"};

                stream.write(text.data(), text.size());
            }

            running_.store(true, std::memory_order::release);

            for (std::size_t i = 0; running_.load(std::memory_order::acquire); i++) {
                std::cout << "Hello service counter: " << i << '\n';
                std::this_thread::sleep_for(100ms);
            }
        }

        /**
         * This function will be called by the SvcHostify routine in another thread and,
         * you can, for example, send a signal to your 'run' routine to stop gracefully.
         */
        void on_stop() {
            printf("A stop signal received.\n");
            printf("Requesting a stop.\n");

            running_.store(false, std::memory_order::release);
        }

    private:
        std::atomic_bool running_;
    };

    test_service service;
} // namespace

extern "C" {
__declspec(dllexport) void refvalue_svchostify_run(std::size_t argc, const char* argv[]) try {
    service.run(argc, argv);
} catch (std::exception& ex) {
    std::cout << ex.what() << '\n';
}

__declspec(dllexport) void refvalue_svchostify_on_stop() try { service.on_stop(); } catch (std::exception& ex) {
    std::cout << ex.what() << '\n';
}
}
