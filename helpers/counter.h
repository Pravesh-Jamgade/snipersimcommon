#ifndef COUNTER
#define COUNTER

#include "fixed_types.h"

class Counter{
    UInt64 count;
    public:
    Counter(int init=0):count(init){}
    UInt64 getCount(){return count;}
    void increase(UInt64 byK=1){count+=byK;}
};

#endif