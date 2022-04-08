#ifndef HELPER_H
#define HELPER_H

#include<iostream>
#include<vector>
#include<unordered_map>
#include "pqueue.h"
#include "fixed_types.h"
#include "mem_component.h"

namespace Helper
{
    class PQueue
    {
        pqueue_t* pQ;
        public:
        PQueue(){
            // pQ=pqueue_init()
        }
        void insert();
        void peek();
        void remove();
        void update();
    };

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
        String source;
        UInt64 totalAccess, totalMiss;
        // coreid, miss_ratio, source memory, totalAccess, totalMisss  
        Message(int core_id, double miss_ratio, String source, UInt64 totalAccess, UInt64 totalMiss):
        core_id(core_id), miss_ratio((double)totalMiss/(double)totalAccess), 
        source(source),totalAccess(totalAccess), totalMiss(totalMiss)
        {}
        int getCore(){return core_id;}
        double getMissRatio(){return miss_ratio;}
        String getName(){return source;}
        UInt64 gettotalMiss(){return totalMiss;}
        UInt64 gettotalAccess(){return totalAccess;}
        double getMiss2HitRatio(){return (double)totalMiss/ (double)(totalAccess-totalMiss);}
        double getMissRatio(){return miss_ratio;}
        UInt64 gettotalHits(){return totalAccess-totalMiss;}
    };

    class MsgCollector
    {
        std::vector<Message> messages;
        public:
        void push(Message msg){messages.push_back(msg);}
        std::vector<Message> getMsg(){return messages;}
    };

    class LevelPredictor
    {   
        std::vector<bool>levelToSkip;
        public:
        LevelPredictor(int size)
        {
            levelToSkip=std::vector<bool>(size,0);
        }
        void addLevelMiss(int level)
        {
            levelToSkip[level]=1;
        }
        void oredLevelMiss(int level)
        {
            levelToSkip[level]=levelToSkip[level]==1?1:0;
        }
        std::vector<bool> getLevelPred(){return levelToSkip;}
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
        PCStatHelper(){}
        typedef std::unordered_map<int, PCStat> LevelPCStat;
        //epoc based but reset for next epoc usage
        std::unordered_map<IntPtr, LevelPCStat> tmpAllLevelPCStat;
        std::unordered_map<IntPtr, LevelPredictor> tmpAllLevelLP;
        //epoc based but cumulative
        std::unordered_map<IntPtr, LevelPCStat> globalAllLevelPCStat;
        LevelPredictor globalAllLevelLP;

        void reset(){tmpAllLevelPCStat.erase(tmpAllLevelPCStat.begin(), tmpAllLevelPCStat.end());}
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

        void insertEntry(int level, IntPtr pc, bool cache_hit){
            
            if(level>=20)
            {
                printf("%s, %ld\n", MemComponent2String(static_cast<MemComponent::component_t>(level)).c_str(), pc);
            }
            if(level<=2){
                return;
            }
            insert(tmpAllLevelPCStat, level,pc,cache_hit);            
            insert(globalAllLevelPCStat, level,pc,cache_hit);            
            insertEntry(level-1, pc, false);
        }

        std::vector<Message> getMessage(IntPtr pc, std::unordered_map<IntPtr, LevelPCStat>& mp){
            std::vector<Message> allMsg;
            tmpAllLevelLP[pc]=LevelPredictor(3);

            bool firstRatio=true; 
            double preRatio, currRatio, prevMisses;
            for(auto uord: mp[pc])
            {
                PCStat pcStat= uord.second;
                UInt64 total = pcStat.getHitCount() + pcStat.getMissCount();
                Message msg = Message(-1,-1, MemComponent2String(static_cast<MemComponent::component_t>(uord.first)), total, pcStat.getMissCount());
                allMsg.push_back(msg);
                if(firstRatio){
                    currRatio=msg.gettotalMiss()/msg.gettotalAccess();
                    firstRatio=false;
                }
                else{
                    currRatio=((double)msg.gettotalMiss()/(double)preMisses) * 100;
                } 

                prevMisses=msg.gettotalMiss();
                if( abs(msg.gettotalHits()-msg.gettotalMiss()) > 500 ){
                    if(msg.getMissRatio() > 0.5){
                        tmpAllLevelLP[pc].addLevelMiss();
                    }
                }

            }
            return allMsg;
        }
    };


}
#endif