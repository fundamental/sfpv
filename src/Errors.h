#define __STDC_CONSTANT_MACROS 1
#define __STDC_FORMAT_MACROS 1
#define __STDC_LIMIT_MACROS 1
#include <clang/Basic/Diagnostic.h>
extern const int error_realtime_saftey_violation;
extern const int error_realtime_saftey_trace;
extern const int error_realtime_saftey_trace_end;
extern const int error_realtime_safety_class;
extern const int warnn_realtime_saftey_unknown;
void errors_init(clang::DiagnosticsEngine &e);
