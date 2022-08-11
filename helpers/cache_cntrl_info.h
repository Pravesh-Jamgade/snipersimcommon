#ifndef CCI_H
#define CCI_H
#include<iostream>
#include"fixed_types.h"
#include "log.h"

class CCI{
    public:
    UInt64 access=0, hits=0, misses=0;
    double miss_ratio=0;

    void clear(){
        access=hits=misses=0;
        miss_ratio=0;
    }

    void log(UInt64 epoc, String name, UInt64 deadblocks, UInt64 evicts){
        miss_ratio=(double)misses/(double)access;
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::DEBUG, "%ld, %ld, %ld, %ld, %lf, %ld, %ld, %s\n", 
            epoc, access, hits, misses, miss_ratio, deadblocks, evicts, name.c_str());
    }

};


#endif