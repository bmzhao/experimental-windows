#include "stacktrace/stacktrace.h"

#include <string>
#include <vector>

#include <windows.h>

#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
// #pragma comment(lib, "kernel32.lib")

namespace experimental {

namespace {

bool SymbolsAreAvailable() {
  // We initialize the Symbolizer:
  // https://docs.microsoft.com/en-us/windows/win32/debug/initializing-the-symbol-handler
  static bool kSymbolsAvailable = []() {
    // Note that according to
    // https://docs.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-symcleanup#remarks,
    // we should call SymCleanup at process end.
    std::atexit([]() { SymCleanup(GetCurrentProcess()); });
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    return SymInitialize(GetCurrentProcess(), NULL, true);
  }();
  return kSymbolsAvailable;
}

}  // namespace

// Note that much of this implementation was taken from
// https://docs.microsoft.com/en-us/windows/win32/debug/about-dbghelp
// And the individual tutorials here:
// https://docs.microsoft.com/en-us/windows/win32/debug/initializing-the-symbol-handler
// Todo(bmzhao): Add additional synchronization???
std::string CurrentStackTrace() {
  // https://docs.microsoft.com/en-us/windows/win32/debug/capturestackbacktrace
  HANDLE current_process = GetCurrentProcess();
  static constexpr int kMaxStackFrames = 64;
  void* trace[kMaxStackFrames];
  int num_frames = CaptureStackBackTrace(0, kMaxStackFrames, trace, NULL);

  // Converting an address to a symbol is documented here:
  // https://docs.microsoft.com/en-us/windows/win32/debug/retrieving-symbol-information-by-address
  std::string stacktrace;
  for (int i = 0; i < num_frames; ++i) {
    const char* symbol = "(unknown)";
    if (SymbolsAreAvailable()) {
      char symbol_info_buffer[sizeof(SYMBOL_INFO) +
                              MAX_SYM_NAME * sizeof(TCHAR)];
      SYMBOL_INFO* symbol_ptr =
          reinterpret_cast<SYMBOL_INFO*>(symbol_info_buffer);
      symbol_ptr->SizeOfStruct = sizeof(SYMBOL_INFO);
      symbol_ptr->MaxNameLen = MAX_SYM_NAME;
      if (SymFromAddr(current_process, reinterpret_cast<DWORD64>(trace[i]), 0,
                      symbol_ptr)) {
        symbol = symbol_ptr->Name;
      }
    }

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "0x%p\t%s", trace[i], symbol);
    stacktrace += buffer;
    stacktrace += "\n";
  }

  return stacktrace;
}

}  // namespace experimental
