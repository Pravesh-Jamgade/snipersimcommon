#ifndef _EM_
#define _EM_

#include "fixed_types.h"

namespace EpocManagerSpace
{
    class EpocManager
    {   
        UInt64 epocLength, prevCycle;
        
        public:
        UInt64 number;
        EpocManager(UInt64 epocLength){
            epocLength=epocLength;
            prevCycle=0;
            number=0;
        }
        UInt64 getEpoc(){return number;}
        bool IsEpocEnded(UInt64 cycle){
            bool epoc=false;
            UInt64 modCycle = cycle%epocLength;
            if(modCycle<prevCycle){
                number++;
                epoc=true;
            }
            prevCycle=modCycle;
            return epoc;
        }
    };
}

#endif