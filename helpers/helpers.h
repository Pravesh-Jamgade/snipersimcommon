#ifndef HELPER_H
#define HELPER_H

#include<iostream>
#include<vector>
#include<unordered_map>
#include<map>
#include "pqueue.h"
#include "fixed_types.h"
#include "mem_component.h"
#include "log.h"

namespace Helper
{
    class Counter
    {
        UInt64 count;
        UInt64 reuse;

        public:
        
        Counter(int init=0){this->count=init, this->reuse=init;}
        void increase(){this->count++;}
        void increase(UInt64 byk){this->count=this->count+byk;}
        UInt64 getCount(){return this->count;}
        void decrease(){this->count--;}
        void decrease(UInt64 byk){this->count=this->count-byk;}
        void reset(){this->count=this->reuse;}
        bool operator!=(int a){
            return this==0;
        }
    };

    class Message
    {
        public:
        int core_id;
        double miss_ratio;
        MemComponent::component_t source;
        UInt64 totalAccess, totalMiss;
        bool levelSkipable;
        // coreid, miss_ratio, source memory, totalAccess, totalMisss  
        Message(int core_id, double miss_ratio, MemComponent::component_t source, UInt64 totalAccess, UInt64 totalMiss):
        core_id(core_id), miss_ratio((double)totalMiss/(double)totalAccess), 
        source(source),totalAccess(totalAccess), totalMiss(totalMiss),levelSkipable(false)
        {}
        int getCore(){return core_id;}
        UInt64 gettotalMiss(){return totalMiss;}
        UInt64 gettotalAccess(){return totalAccess;}
        double getMiss2HitRatio(){return (double)totalMiss/ (double)(totalAccess-totalMiss);}
        double getMissRatio(){return miss_ratio;}
        UInt64 gettotalHits(){return totalAccess-totalMiss;}
        MemComponent::component_t getLevel(){return source;}
        void addLevelSkip(){levelSkipable=true;}
        bool isLevelSkipable(){return levelSkipable;}
    };
}
#endif