class GraphBuilder;

class ReverseDeductions
{
    public:
        ReverseDeductions(GraphBuilder *gb);
        ~ReverseDeductions(void);
        void print(void);
        void print_whitelist(void);
    private:
        class ReverseDeductionsImpl *impl;
};

