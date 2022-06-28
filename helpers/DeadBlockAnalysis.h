#ifndef DEAD_BLOCK_ANALYSIS
#define DEAD_BLOCK_ANALYSIS

#include<iostream>
#include<set>
#include"helpers.h"
#include "log.h"
#include<fstream>
#include <math.h>  
#include "fixed_types.h"
#include<memory>

namespace DeadBlockAnalysisSpace
{
    class BlockInfo{
        public:
        std::set<IntPtr> evictList;
        std::set<IntPtr> uniqueList;

        void addToUniqueList(IntPtr addr){
             auto findAddr = uniqueList.find(addr);
             if(findAddr==uniqueList.end()){
                 uniqueList.insert(addr);
             }
        }

        UInt64 countUniqueList(){
            return uniqueList.size();
        }

        bool missFromPrevLevelFound(IntPtr addr){
            auto findAddr = evictList.find(addr);
            if(findAddr!=evictList.end()){
                return true;
            }
            return false;
        }

        void addEvicted(IntPtr addr){
            if(missFromPrevLevelFound(addr)){
                return;
            }
            else evictList.insert(addr);
        }

        void eraseEntryIffound(IntPtr addr){
            if(missFromPrevLevelFound(addr)){
                evictList.erase(addr);
            }
        }

        UInt64 countEvictList(){
            return evictList.size();
        }
    };
}
#endif