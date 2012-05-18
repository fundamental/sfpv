#define REALTIME __attribute__((annotate("RT")))
#define NREALTIME __attribute__((annotate("!RT")))

void undefined_function(void);

void bar(void)
{}

void REALTIME foo(int x)
{
    (void) x;
    bar();
    undefined_function();
}


void NREALTIME baz(void)
{}

int tall_number;

int REALTIME main()
{
    int barbar;
    foo(barbar);
    baz();
    return 0;
}
