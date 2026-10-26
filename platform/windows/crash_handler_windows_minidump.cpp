/**************************************************************************/
/*  crash_handler_windows_minidump.cpp                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "crash_handler_windows.h"

#include "core/os/os.h"
#include "core/string/print_string.h"
#include "core/string/ustring.h"

#include <dbghelp.h>

#include <atomic>

// Tessary fork addition: a minidump for every crash, in release builds too.
//
// The backtrace handlers in crash_handler_windows_{seh,signal}.cpp only exist with
// DEBUG_ENABLED, so a release template used to die with no record at all. This writer is
// always compiled and always installed (CrashHandler::initialize): an unhandled-exception
// filter covers release builds, and the DEBUG_ENABLED backtrace handlers call the same
// writer before printing their backtrace. A once-flag keeps it to one dump per crash.
//
// The dump type is deliberately not a full-memory dump: thread info plus indirectly
// referenced memory is enough to symbolise every thread's stack (the render thread
// included) against the shipped .debugsymbols, at a fraction of the size. The file lands
// beside the log (<user data dir>/logs/crash-<UTC time>-<pid>.dmp) and its path is printed
// to the log as "[crash] minidump: ...", where the game's next-launch crash capture finds
// it and preserves it with the log.

namespace {

std::atomic<bool> minidump_done{ false };
LPTOP_LEVEL_EXCEPTION_FILTER previous_filter = nullptr;

String minidump_path() {
	SYSTEMTIME now;
	GetSystemTime(&now);
	String name = vformat("crash-%04d%02d%02d-%02d%02d%02d-%d.dmp",
			(int)now.wYear, (int)now.wMonth, (int)now.wDay,
			(int)now.wHour, (int)now.wMinute, (int)now.wSecond,
			(int)GetCurrentProcessId());
	String dir;
	if (OS::get_singleton() != nullptr) {
		dir = OS::get_singleton()->get_user_data_dir();
	}
	if (dir.is_empty()) {
		return name;
	}
	dir = dir.path_join("logs");
	// The file logger normally created this already; make sure in case we crash earlier.
	Char16String dir16 = dir.utf16();
	CreateDirectoryW((LPCWSTR)dir16.get_data(), nullptr);
	return dir.path_join(name);
}

LONG WINAPI crash_minidump_filter(EXCEPTION_POINTERS *ep) {
	WriteCrashMinidump(ep);
	// Let the normal path continue: the DEBUG_ENABLED backtrace (if any) already ran or
	// runs through the signal machinery, and the OS still terminates the process.
	return EXCEPTION_CONTINUE_SEARCH;
}

} // namespace

void WriteCrashMinidump(EXCEPTION_POINTERS *ep) {
	if (minidump_done.exchange(true)) {
		return;
	}
	if (OS::get_singleton() != nullptr && OS::get_singleton()->is_disable_crash_handler()) {
		return;
	}

	String path = minidump_path();
	Char16String path16 = path.utf16();
	HANDLE file = CreateFileW((LPCWSTR)path16.get_data(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE) {
		print_error(vformat("[crash] minidump FAILED: cannot create %s (error %d)", path, (int)GetLastError()));
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION exception_info;
	exception_info.ThreadId = GetCurrentThreadId();
	exception_info.ExceptionPointers = ep;
	exception_info.ClientPointers = FALSE;
	const MINIDUMP_TYPE type = (MINIDUMP_TYPE)(MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory);
	BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type,
			ep != nullptr ? &exception_info : nullptr, nullptr, nullptr);
	DWORD error = ok ? 0 : GetLastError();
	CloseHandle(file);
	if (ok) {
		print_error(vformat("[crash] minidump: %s", path));
	} else {
		print_error(vformat("[crash] minidump FAILED: MiniDumpWriteDump error %d (%s)", (int)error, path));
		Char16String failed16 = path.utf16();
		DeleteFileW((LPCWSTR)failed16.get_data());
	}
}

void InstallCrashMinidump() {
	previous_filter = SetUnhandledExceptionFilter(crash_minidump_filter);
}

void UninstallCrashMinidump() {
	SetUnhandledExceptionFilter(previous_filter);
	previous_filter = nullptr;
}
