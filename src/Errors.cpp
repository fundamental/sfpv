#include <clang/Basic/Diagnostic.h>
int error_realtime_saftey_violation;
int error_realtime_saftey_trace;
int error_realtime_saftey_trace_end;
int error_realtime_safety_class;
int warnn_realtime_saftey_unknown;

void errors_init(clang::DiagnosticsEngine &e)
{
    error_realtime_saftey_violation =
        e.getCustomDiagID(clang::DiagnosticsEngine::Error,
                "Realtime safety violation, %0 called from a safe function");

    error_realtime_saftey_trace =
        e.getCustomDiagID(clang::DiagnosticsEngine::Note,
                "%0 deduced realtime safe");

    error_realtime_saftey_trace_end =
        e.getCustomDiagID(clang::DiagnosticsEngine::Note,
                "%0 specified realtime safe by user");

    error_realtime_safety_class =
        e.getCustomDiagID(clang::DiagnosticsEngine::Note,
                "Method %0 deduced safe from superclass (%1)");

    warnn_realtime_saftey_unknown =
        e.getCustomDiagID(clang::DiagnosticsEngine::Warning,
                "Function %0 was deduced to need to be safe, but it was not marked, nor " \
                "did it have a function body to be processed");

}
