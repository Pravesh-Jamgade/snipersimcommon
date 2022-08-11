#ifndef CCI_H
#define CCI_H
#include<iostream>
#include"fixed_types.h"
#include "log.h"

class CCI{

    public:
    UInt64 access, hits, misses;
    double miss_ratio;

    void clear(){
        access=hits=misses=0;
        miss_ratio=0;
    }

    void log(){
        
    }

};


#endif