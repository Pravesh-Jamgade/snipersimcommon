#ifndef _EM_
#define _EM_

#include "fixed_types.h"

namespace EpocManagerSpace
{
    class EpocManager
    {   
        UInt64 epocLength, currCyle;
        
        public:
        UInt64 number;
        EpocManager(UInt64 epocLength, UInt64 currCyle){
            epocLength=epocLength;
            currCyle=currCyle;
            number=0;
        }
        bool IsEpocEnded(UInt64 cycle){
            number++;
            cycle = cycle-currCyle;
            if(cycle<0)
                cycle*=-1;
            return cycle%epocLength == 0;
        }
    };
}

#endif