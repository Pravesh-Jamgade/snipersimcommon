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
        EpocManager(UInt64 epocLength){
            epocLength=epocLength;
            number=0;
        }
        UInt64 getEpoc(){return number;}
        bool IsEpocEnded(UInt64 cycle){
            if(cycle%epocLength == 0){
                number++;
                return true;
            }
            return false;
        }
    };
}

#endif