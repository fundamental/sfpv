#define REALTIME __attribute__((annotate("RT")))
#define NREALTIME __attribute__((annotate("!RT")))
#include "test-input2.h"

class TestClass
{
    void bar(void) {other_thing();};
    void REALTIME foo(void) {bar();};
    void REALTIME entry(void) { foo(); baz(); }
    void NREALTIME baz(void) {};
};

