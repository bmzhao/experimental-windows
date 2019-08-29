#include "stacktrace/stacktrace.h"

#include <string>
#include <vector>

#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")


namespace experimental {

    // Note that much of this implementation was taken from 
    // https://docs.microsoft.com/en-us/windows/win32/debug/about-dbghelp
    // And the individual tutorials here: https://docs.microsoft.com/en-us/windows/win32/debug/initializing-the-symbol-handler
    // Todo(bmzhao): Add additional synchronization???
    std::string CurrentStackTrace() {        
        HANDLE process_handle = GetCurrentProcess();
        HANDLE thread_handle = GetCurrentThread();

        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        if (!SymInitialize(process_handle, NULL, TRUE)) {
            return "Stacktrace not generated. SymInitialize returned error : " + std::to_string(GetLastError()) + "\n";
        }
        
        // Note that we explicitly zero initialize context: https://en.cppreference.com/w/cpp/language/zero_initialization
        CONTEXT context{};
        RtlCaptureContext(&context);
        
        // According to https://docs.microsoft.com/en-us/windows/win32/debug/updated-platform-support, we should
        // prefer the 64 suffixed version of all types.
        // We always use AddrModeFlat: https://docs.microsoft.com/en-us/windows/win32/api/dbghelp/ns-dbghelp-address64
        // We must also initialize AddrPc, AddrFrame, and AddrStack prior to calling StackWalk64
        STACKFRAME64 frame{};
        frame.AddrPC.Offset = context.Rip;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = context.Rbp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Offset = context.Rsp;
        frame.AddrStack.Mode = AddrModeFlat;
        
        // StackWalk64 is documented here: https://docs.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-stackwalk64
        std::vector<DWORD64> stack_frame_ptrs;
        while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, process_handle,
                    thread_handle, &frame, &context, NULL, 
                    SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            stack_frame_ptrs.push_back(frame.AddrPC.Offset);
        }

        // Converting an address to a symbol is documented here:
        // https://docs.microsoft.com/en-us/windows/win32/debug/retrieving-symbol-information-by-address        
        std::string stacktrace;
        for (const auto stack_frame_ptr: stack_frame_ptrs) {
            char symbol_info_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            SYMBOL_INFO* symbol_ptr = reinterpret_cast<SYMBOL_INFO*>(symbol_info_buffer);
            symbol_ptr->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol_ptr->MaxNameLen = MAX_SYM_NAME;        

            const char* symbol = "(unknown)";
            if (SymFromAddr(process_handle, stack_frame_ptr, 0, symbol_ptr)) {
                symbol = symbol_ptr->Name;
            }
            
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "0x%p\t%s", reinterpret_cast<void*>(stack_frame_ptr), symbol);
            stacktrace += buffer;
            stacktrace += "\n";
        }

        SymCleanup(process_handle);
        return stacktrace;
    }
}
