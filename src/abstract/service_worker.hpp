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

#pragma once

#include "../service_config.hpp"

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace essence::win::abstract {
    class service_worker {
    public:
        template <typename T>
            requires(!std::same_as<std::decay_t<T>, service_worker>)
        explicit service_worker(T&& value) : wrapper_{std::make_shared<wrapper<T>>(std::forward<T>(value))} {}

        [[nodiscard]] const service_config& config() const {
            return wrapper_->config();
        }

        void on_start() const {
            wrapper_->on_start();
        }

        void on_stop() const {
            wrapper_->on_stop();
        }

        void run() const {
            wrapper_->run();
        }

    private:
        struct base {
            virtual ~base()                        = default;
            virtual const service_config& config() = 0;
            virtual void on_start()                = 0;
            virtual void on_stop()                 = 0;
            virtual void run()                     = 0;
        };

        template <typename T>
        class wrapper final : public base {
        public:
            template <std::convertible_to<T> U>
            explicit wrapper(U&& value) : value_{std::forward<U>(value)} {}

            const service_config& config() override {
                return value_.config();
            }

            void on_start() override {
                value_.on_start();
            }

            void on_stop() override {
                value_.on_stop();
            }

            void run() override {
                value_.run();
            }

        private:
            T value_;
        };

        std::shared_ptr<base> wrapper_;
    };
} // namespace essence::win::abstract
