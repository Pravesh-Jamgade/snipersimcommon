#ifndef EPOC_H
#define EPOC_H
#include "fixed_types.h"

class EpocHelper
{   
    UInt64 prevCycle, epocLength;
    bool epocEnded;
    UInt64 epocCounter;
    
    public:
    static bool once;
    
    EpocHelper(){
    }
    EpocHelper(UInt64 epocLength){
        this->epocLength=epocLength;
        this->prevCycle=0;
        this->epocCounter=-1;
        this->epocEnded=false;
    }
    void doStatusUpdate(UInt64 cycle){
        UInt64 modCycle=cycle%epocLength;
        if(modCycle<prevCycle){
            epocEnded=true;
            epocCounter++;
        }
        else epocEnded=false;
        prevCycle=modCycle;
    }
    static bool head(){
        if(once){
            once=false;
            return true;
        }
        return false;
    }
    bool getEpocStatus(){return epocEnded;}
    void reset(){epocEnded=false;}
    UInt64 getEpocCounter(){return epocCounter;}
};

#endif