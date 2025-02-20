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

#include <Windows.h>
#include <rpc.h>

module refvalue.svchostify:win32.svchost;

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;

typedef NTSTATUS(WINAPI* LPSTART_RPC_SERVER)(RPC_WSTR, RPC_IF_HANDLE);
typedef NTSTATUS(WINAPI* LPSTOP_RPC_SERVER)(RPC_IF_HANDLE);
typedef NTSTATUS(WINAPI* LPSTOP_RPC_SERVER_EX)(RPC_IF_HANDLE);

typedef VOID(WINAPI* LPNET_BIOS_OPEN)(VOID);
typedef VOID(WINAPI* LPNET_BIOS_CLOSE)(VOID);
typedef DWORD(WINAPI* LPNET_BIOS_RESET)(UCHAR);

typedef DWORD(WINAPI* LPREGISTER_STOP_CALLBACK)(HANDLE* phNewWaitObject, PCWSTR pszServiceName, HANDLE hObject,
    WAITORTIMERCALLBACK Callback, PVOID Context, DWORD dwFlags);

typedef struct _SVCHOST_GLOBAL_DATA {
    PSID NullSid; // S-1-0-0
    PSID WorldSid; // S-1-1-0
    PSID LocalSid; // S-1-2-0
    PSID NetworkSid; // S-1-5-2
    PSID LocalSystemSid; // S-1-5-18
    PSID LocalServiceSid; // S-1-5-19
    PSID NetworkServiceSid; // S-1-5-20
    PSID BuiltinDomainSid; // S-1-5-32
    PSID AuthenticatedUserSid; // S-1-5-11
    PSID AnonymousLogonSid; // S-1-5-7
    PSID AliasAdminsSid; // S-1-5-32-544
    PSID AliasUsersSid; // S-1-5-32-545
    PSID AliasGuestsSid; // S-1-5-32-546
    PSID AliasPowerUsersSid; // S-1-5-32-547
    PSID AliasAccountOpsSid; // S-1-5-32-548
    PSID AliasSystemOpsSid; // S-1-5-32-549
    PSID AliasPrintOpsSid; // S-1-5-32-550
    PSID AliasBackupOpsSid; // S-1-5-32-551
    LPSTART_RPC_SERVER StartRpcServer;
    LPSTOP_RPC_SERVER StopRpcServer;
    LPSTOP_RPC_SERVER_EX StopRpcServerEx;
    LPNET_BIOS_OPEN NetBiosOpen;
    LPNET_BIOS_CLOSE NetBiosClose;
    LPNET_BIOS_RESET NetBiosReset;
    LPREGISTER_STOP_CALLBACK RegisterStopCallback;
} SVCHOST_GLOBAL_DATA;
