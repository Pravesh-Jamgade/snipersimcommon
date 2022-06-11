#ifndef _EM_
#define _EM_

#include "fixed_types.h"

namespace EpocManagerSpace
{
    class EpocManager
    {   
        UInt64 epocLength;
        UInt64 reset, prev;
        public:
        UInt64 number;
        EpocManager(UInt64 epocLength){
            this->reset=this->epocLength=epocLength;
            prev=number=0;
        }
        UInt64 getEpoc(){return number;}
        bool IsEpocEnded(UInt64 cycle){
            if(cycle%epocLength==0 && prev != cycle){
                prev=cycle;
                return true;
            }
            return false;
        }
    };
}

#endif