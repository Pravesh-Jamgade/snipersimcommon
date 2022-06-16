#ifndef EPOC_H
#define EPOC_H
#include "fixed_types.h"
class EpocHelper
{   
    UInt64 prevCycle, epocLength;
    static bool epocEnded;
    public:
    EpocHelper(){
    }
    EpocHelper(UInt64 epocLength){
        this->epocLength=epocLength;
        this->prevCycle=0;
    }
    void doStatusUpdate(UInt64 cycle){
        UInt64 modCycle=cycle%epocLength;
        if(modCycle<prevCycle){
            epocEnded=true;
        }
        else epocEnded=false;
        prevCycle=modCycle;
    }
    
    static bool getEpocStatus(){
        return epocEnded;
    }
    static void reset(){epocEnded=false;}
};
#endif