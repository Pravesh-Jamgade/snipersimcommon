#ifndef MYHELP
#define MYHELP


#include "branch_predictor.h"

#include <vector>

class OneBitPredictor:public BranchPredictor
{
    public:
    OneBitPredictor(String name, core_id_t core_id, int size):BranchPredictor(name, core_id), table(size, false){}
    ~OneBitPredictor(){}

    UInt32 getIndex(IntPtr ip)
    {
        UInt32 ret = ip>>6 & 0x1ff;
        if(ret>=512){
            fprintf(stderr, "%ld", ret);
            fprintf(stderr, "%s", "index is above size\n");
            exit(0); 
        }
        return ret;
    }

    bool predict(bool indirect, IntPtr ip, IntPtr target)
    {
        UInt32 index = getIndex(ip);
        return table[index];
    }
    void update(bool predicted, bool actual, bool indirect, IntPtr ip, IntPtr target)
    {
        updateCounters(predicted, actual);
        UInt32 index = getIndex(ip);
        table[index]=actual;
    }

    private:
    std::vector<bool> table;
};
#endif