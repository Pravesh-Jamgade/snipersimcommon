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
            UInt64 mod = cycle%epocLength;
            bool status=false;
            if(prev>mod){
                number++;
                status=true;
            }
            prev=mod;
            return status;
        }
    };
}

#endif