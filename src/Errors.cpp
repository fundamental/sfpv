#define __STDC_CONSTANT_MACROS 1
#define __STDC_FORMAT_MACROS 1
#define __STDC_LIMIT_MACROS 1
#include <clang/Basic/Diagnostic.h>
int error_realtime_saftey_violation;
int error_realtime_saftey_trace;
int error_realtime_saftey_trace_end;
int warnn_realtime_saftey_unknown;

void errors_init(clang::DiagnosticsEngine &e)
{
    error_realtime_saftey_violation =
        e.getCustomDiagID(clang::DiagnosticsEngine::Error,
                "Realtime saftey violation, %0 Called from a safe function");

    error_realtime_saftey_trace =
        e.getCustomDiagID(clang::DiagnosticsEngine::Note,
                "%0 Deduced Realtime safe");

    error_realtime_saftey_trace_end =
        e.getCustomDiagID(clang::DiagnosticsEngine::Note,
                "%0 Specified Realtime safe by user");

    warnn_realtime_saftey_unknown =
        e.getCustomDiagID(clang::DiagnosticsEngine::Warning,
                "Function %0 was deduced to need to be safe, but it was not marked, nor " \
                "did it have a funciton body to be processed");
}
