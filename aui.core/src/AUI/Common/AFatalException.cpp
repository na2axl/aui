// AUI Framework - Declarative UI toolkit for modern C++20
// Copyright (C) 2020-2023 Alex2772
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library. If not, see <http://www.gnu.org/licenses/>.

//
// Created by alex2772 on 1/14/22.
//

#include "AFatalException.h"
#include "AUI/Traits/memory.h"
#include "AUI/Logging/ALogger.h"

AFatalException::Handler& AFatalException::handler() {
    static Handler h;
    return h;
}

AString AFatalException::getMessage() const noexcept {
    return "{} at address {}"_format(mSignalName, mAddress);
}

#ifdef AUI_CATCH_UNHANDLED

extern "C" {
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#if !AUI_PLATFORM_WIN

extern "C" {
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <execinfo.h>
#include <dlfcn.h>
}

#include <cxxabi.h>
void restoreRt() {
    printf("restoreRt");
}
static void unblockSignal(int signum __attribute__((__unused__)))
{
    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, signum);
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
}
static void onSignal(int c, siginfo_t * info, void *_p __attribute__ ((__unused__))) {
    const char* signalName = strsignal(c);
    if (!signalName) signalName = "unknown signal";
    AFatalException e(signalName, c);

    ALogger::err("SignalHandler") << "Caught signal: " << signalName << "(" << c << ")\n" << AStacktrace::capture(3);

    switch (c) {
        default:
            break;
        case SIGILL:
        case SIGFPE:
        case SIGSEGV:
        case SIGABRT:
            unblockSignal(c);
#if !AUI_COMPILER_CLANG
            throw e;
#endif
    }
}

#else

static void __cdecl onSignal(int c) {
    const char* signalName = "unknown signal";

    switch (c) {
        case SIGILL:
            signalName = "Illegal instruction";
            break;
        case SIGFPE:
            signalName = "floating point exception";
            break;
        case SIGSEGV:
            signalName = "access violation";
            break;
        case SIGABRT:
            signalName = "abort";
            break;
    }
    AFatalException e(signalName, c);

    ALogger::err("SignalHandler") << "Caught signal: " << signalName << "(" << c << ")\n" << AStacktrace::capture(3);

    throw e;
}
#endif

void aui_init_signal_handler() {
#if !AUI_PLATFORM_WIN
    struct sigaction act;
    aui::zero(act);
    act.sa_sigaction = onSignal;
    sigemptyset (&act.sa_mask);
    act.sa_flags = SA_SIGINFO|0x4000000;
#ifdef __linux
    act.sa_restorer = restoreRt;
#endif
    //auto r = syscall(SYS_rt_sigaction, SIGSEGV, &act, nullptr, _NSIG / 8);
    //assert(r == 0);
    sigaction(SIGILL, &act, nullptr);
    sigaction(SIGFPE, &act, nullptr);
    sigaction(SIGSEGV, &act, nullptr);
    sigaction(SIGABRT, &act, nullptr); // for assertions
#else

    signal(SIGILL, onSignal);
    signal(SIGFPE, onSignal);
    signal(SIGSEGV, onSignal);
    signal(SIGABRT, onSignal); // for assertions
#endif
}

#endif


AFatalException::AFatalException(std::string_view signalName, int nativeSignalId) :
        AException(AStacktrace::capture(3)
#ifdef AUI_CATCH_UNHANDLED
    .stripBeforeFunctionCall(reinterpret_cast<void*>(onSignal))
#endif
        ),
        mSignalName(signalName), mNativeSignalId(nativeSignalId) // avoiding unrecommended operations as much as possible
{
    mAddress = [&]() -> void* {
        const auto& e = stacktrace().entries();
        if (e.empty()) {
            return nullptr;
        }
        return e.first().ptr();
    }();

    if (handler())
        handler()(this);
}