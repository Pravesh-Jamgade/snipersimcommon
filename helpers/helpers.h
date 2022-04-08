#ifndef HELPER_H
#define HELPER_H

#include<iostream>
#include<vector>
#include<unordered_map>
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
        
        Counter(int init=1){this->count=init, this->reuse=init;}
        void increase(){this->count++;}
        void increase(UInt64 byk){this->count=this->count+byk;}
        UInt64 getCount(){return this->count;}
        void decrease(){this->count--;}
        void decrease(UInt64 byk){this->count=this->count-byk;}
        void reset(){this->count=this->reuse;}
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

    class LevelPredictor
    {   
        UInt64 levelToSkip;
        public:
        LevelPredictor(){}
        void addSkipLevel(MemComponent::component_t level)
        {
            int a=1;
            a=1<<level;
            if(!canSkipLevel(level))
                levelToSkip=levelToSkip ^ a;
        }

        bool canSkipLevel(MemComponent::component_t level){
            int a=1;
            a=1<<level;
            return levelToSkip & a;
        }
    };

    class PCStat
    {
        Counter hit, miss;
        public:
        PCStat()
        {
            hit=Counter();
            miss=Counter();
        }      
        void increaseMiss(){miss.increase();}
        void increaseHit(){hit.increase();}
        UInt64 getHitCount(){return hit.getCount();}
        UInt64 getMissCount(){return miss.getCount();}
        double getMissRatio(){return (double)miss.getCount()/(double)(miss.getCount()+hit.getCount()); }
        double getMissHitRate(){return (double)miss.getCount()/(double)(hit.getCount()); }
    };

    class PCStatHelper
    {
        public:
        PCStatHelper(){
            lp_unlock=false;
        }
        typedef std::unordered_map<int, PCStat> LevelPCStat;
        //epoc based but reset for next epoc usage
        std::unordered_map<IntPtr, LevelPCStat> tmpAllLevelPCStat;
        std::unordered_map<IntPtr, LevelPredictor> tmpAllLevelLP;
        //epoc based but cumulative
        std::unordered_map<IntPtr, LevelPCStat> globalAllLevelPCStat;
        LevelPredictor globalAllLevelLP;

        // LP in-use after 1st epoc
        bool lp_unlock;

        void reset(){
            tmpAllLevelPCStat.erase(tmpAllLevelPCStat.begin(), tmpAllLevelPCStat.end());
            lp_unlock=true;    
        }
        int getTmpSize(){ return tmpAllLevelPCStat.size();}
        int getGlobalSize(){return globalAllLevelPCStat.size();}

        void insert(std::unordered_map<IntPtr, LevelPCStat>& tmpAllLevelPCStat, int level, IntPtr pc, bool cache_hit){

            auto findPc = tmpAllLevelPCStat.find(pc);
            if(findPc!=tmpAllLevelPCStat.end()){
               
                auto findLevel=tmpAllLevelPCStat[pc].find(level);
                if(findLevel!=tmpAllLevelPCStat[pc].end()){
                    if(cache_hit)
                        findLevel->second.increaseHit();
                    else findLevel->second.increaseMiss();
                }
                else{
                    tmpAllLevelPCStat[pc].insert({level,PCStat()});
                }
            }
            else{
                LevelPCStat levelPCStat;
                levelPCStat.insert({level,PCStat()});
                tmpAllLevelPCStat.insert({pc,levelPCStat});
            }
        }

        std::vector<MemComponent::component_t> LPPrediction(IntPtr pc){
            std::vector<MemComponent::component_t> predicted_levels;
            // LP prediction
            if(lp_unlock)
            {
                auto ptr = tmpAllLevelLP.find(pc);
                if(ptr!=tmpAllLevelLP.end()){
                    for(int i=3;i<= 5; i++){// (5-3+1)=3 level's of cache
                        if(ptr->second.canSkipLevel(static_cast<MemComponent::component_t>(i) )){
                            predicted_levels.push_back(static_cast<MemComponent::component_t>(i));
                        }
                    }
                }
            }
            return predicted_levels;
        }

        bool LPPredictionVerifier(IntPtr pc, MemComponent::component_t actual_level){
            if(lp_unlock){
                for(auto e: LPPrediction(pc)){
                    if(e==actual_level){
                        _LOG_CUSTOM_LOGGER(Log::Warning,Log::LogDst::LP_Prediction_Match,"%ld,%s\n",pc,MemComponent2String(e).c_str() );
                        return true;
                    }
                }
            }
            return false;
        }

        void insertEntry(int level, IntPtr pc, bool cache_hit){
            
            if(level<=2){
                return;
            }
            
            insert(tmpAllLevelPCStat, level,pc,cache_hit);            
            insert(globalAllLevelPCStat, level,pc,cache_hit);            
            insertEntry(level-1, pc, false);
        }

        // will return epoc based pc stat to log, as well it adds skipable levels;
        std::vector<Message> getMessage(IntPtr pc, std::unordered_map<IntPtr, LevelPCStat>& mp){
            std::vector<Message> allMsg;
            tmpAllLevelLP[pc]=LevelPredictor();

            bool firstRatio=true; 
            double preRatio, currRatio, prevMisses;
            for(auto uord: mp[pc])
            {
                PCStat pcStat= uord.second;
                UInt64 total = pcStat.getHitCount() + pcStat.getMissCount();
                Message msg = Message(-1,-1, static_cast<MemComponent::component_t>(uord.first), total, pcStat.getMissCount());
                allMsg.push_back(msg);
                if(firstRatio){
                    currRatio=msg.gettotalMiss()/msg.gettotalAccess();
                    firstRatio=false;
                }
                else{
                    currRatio=((double)msg.gettotalMiss()/(double)prevMisses) * 100;
                } 

                prevMisses=msg.gettotalMiss();
                if( abs( (long long) (msg.gettotalHits()-msg.gettotalMiss())) > 500 ){//if differencce betn miss and hits for x level is > 500 the go check miss ratio
                    if(currRatio > 0.5){// reason is that it should be significant to skip
                        tmpAllLevelLP[pc].addSkipLevel(msg.getLevel());
                        msg.addLevelSkip();
                    }
                }

            }
            return allMsg;
        }
    };


}
#endif