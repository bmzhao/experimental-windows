#include "stacktrace/stacktrace.h"

#include <string>

#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")


namespace experimental {
    // Note that much of this implementation was taken from 
    std::string CurrentStackTrace() {    
        return "hello world";
        // DWORD error;
        // HANDLE hProcess = GetCurrentProcess();
        // SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        // if (!SymInitialize(hProcess, NULL, TRUE))
        // {
        //     // SymInitialize failed
        //     error = GetLastError();
        //     printf("SymInitialize returned error : %d\n", error);
        //     return FALSE;
        // }        
    }
}
