#ifndef _EM_
#define _EM_

#include "fixed_types.h"

namespace EpocManagerSpace
{
    class EpocManager
    {   
        UInt64 epocLength, reuse;
        public:
        EpocManager(UInt64 epocLength){
            reuse=epocLength=epocLength;
        }
        bool IsEpocEnded(){
            return epocLength==0;
        }
        void decrease(){epocLength--;}
        void reset(){epocLength=reuse;}
    };
}

#endif