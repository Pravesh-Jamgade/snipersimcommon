#ifndef twobit
#define twobit

#include "branch_predictor.h"
#include <vector>
#include<math.h>
namespace TwobitSpace
{

class Counter
{
    public:
    Counter(int bits){
        this->counter=pow(2,bits);
        this->mid=abs(this->counter/2);
        this->upper=this->counter-1;
        this->lower=0;
        this->counter=0;
    }

    void increment(){
        if(counter!=upper)
            counter++;
    }
    void decrement(){
        if(counter!=lower)
            counter--;
    }

    bool predict()
    {
        return counter>=mid;
    }

    bool IsWeakTaken()
    {
        return counter>=mid;
    }

    bool IsWeakNotTaken()
    {
        return counter<mid;
    }

    private:
    int counter;
    int mid,upper,lower;
};

class TwoBitPredictor:public BranchPredictor
{
    public:
    TwoBitPredictor(String name, core_id_t core_id, int size, int counter_bits):BranchPredictor(name, core_id),table(size, Counter(counter_bits)){
        this->size=size;
    }
    ~TwoBitPredictor(){}

    UInt32 getIndex(IntPtr ip)
    {
        UInt32 ret = ip>>6 & 0x1ff;
        // if you change size other than 512 make sure to 
        // change 0x1ff as it is responsible to generate 9 bit(max 512) index
        if(ret>=size){
            fprintf(stderr, "%ld", ret);
            fprintf(stderr, "%s", "index is above size\n");
            exit(0); 
        }
        return ret;
    }

    bool predict(bool indirect, IntPtr ip, IntPtr target)
    {
        return table[getIndex(ip)].predict();
    }

    void update(bool predicted, bool actual, bool indirect, IntPtr ip, IntPtr target)
    {   
        UInt32 index = getIndex(ip);
        updateCounters(predicted,actual);
        if(actual) table[index].increment();
        else table[index].decrement();
            
    }
    std::vector<Counter> table;
    int size;
};
};
#endif