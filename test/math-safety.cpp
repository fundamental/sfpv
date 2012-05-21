#include <cmath>
#define REALTIME __attribute__((annotate("RT")))
#define NREALTIME __attribute__((annotate("!RT")))

void REALTIME test_functions()
{
    float f = 23.3;
    sinf(f);
    cosf(f);
    powf(f,f);
}

int main()
{
}
