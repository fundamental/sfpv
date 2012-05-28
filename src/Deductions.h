class GraphBuilder;

class Deductions
{
    public:
        Deductions(GraphBuilder *gb);
        ~Deductions(void);
        void print(void);
        int find_inconsistent(GraphBuilder *gb);
    private:
        class DeductionsImpl *impl;
};

