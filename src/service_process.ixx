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

export module refvalue.svchostify:service_process;
import :abstract.service_worker;
import essence.basic;
import std;

export namespace essence::win {
    class service_process {
    public:
        service_process(const service_process&)     = delete;
        service_process(service_process&&) noexcept = delete;
        ~service_process();
        service_process& operator=(const service_process&)     = delete;
        service_process& operator=(service_process&&) noexcept = delete;
        static const service_process& instance();
        void init(zwstring_view service_name) const;
        void run(abstract::service_worker worker) const;
        void report_stopped() const;
        void set_global_data(const void* data) const noexcept;

    private:
        service_process();

        class impl;

        std::unique_ptr<impl> impl_;
    };
} // namespace essence::win
