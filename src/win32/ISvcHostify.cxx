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

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#include <Unknwnbase.h>
#include <comdef.h>

module refvalue.svchostify:win32.ISvcHostify;

struct __declspec(novtable, uuid("CB62E85F-0C69-C76B-E955-655E0D184E5A")) ISvcHostify : IUnknown {
    virtual STDMETHODIMP Run(SAFEARRAY* args) = 0;
    virtual STDMETHODIMP OnStop()             = 0;
};

_COM_SMARTPTR_TYPEDEF(ISvcHostify, __uuidof(ISvcHostify));
