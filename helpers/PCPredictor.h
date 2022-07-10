#ifndef PCPRED_H
#define PCPRED_H

#include<iostream>
#include<vector>
#include<unordered_map>
#include<unordered_set>
#include<map>
#include "pqueue.h"
#include "fixed_types.h"
#include "mem_component.h"
#include "log.h"
#include "helpers.h"
#include "algorithm"
#include <memory>

namespace PCPredictorSpace
{
    class LevelPredictor
    {   
        UInt64 levelToSkip;
        public:
        LevelPredictor(int level){
            levelToSkip=1<<level;
        }
        LevelPredictor(){
            levelToSkip=0;
        }
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

        std::vector<MemComponent::component_t> getLevelStatus(){
            std::vector<MemComponent::component_t> allLevels;
            //TODO:remove these constant values
            for(int i=3;i<=5;i++){
                if(canSkipLevel(static_cast<MemComponent::component_t>(i))){
                    allLevels.push_back(static_cast<MemComponent::component_t>(i));
                }
            }
            return allLevels;
        }
    };

    class PCStat
    {
        Helper::Counter hit, miss;
        public:
        PCStat()
        {
            hit=Helper::Counter();
            miss=Helper::Counter();
        }
        PCStat(bool cache_hit)
        {
            hit=Helper::Counter();
            miss=Helper::Counter();
            if(cache_hit)
                hit.increase();
            else miss.increase();
        }       
        void increaseMiss(){miss.increase();}
        void increaseHit(){hit.increase();}
        UInt64 getHitCount(){return hit.getCount();}
        UInt64 getMissCount(){return miss.getCount();}
        double getMissRatio(){return (double)miss.getCount()/(double)(miss.getCount()+hit.getCount()); }
        double getHitRatio(){return (double)hit.getCount()/(double)(miss.getCount()+hit.getCount()); }
        double getMissHitRate(){return (double)miss.getCount()/(double)(hit.getCount()); }
        UInt64 getTotalCount(){return hit.getCount()+miss.getCount();}
    };

    class LPPerf
    {
        Helper::Counter trueSkip,trueNoSkip,falseSkip, falseNoSkip, total;
        public:
        enum State{
            fs=0,
            ts,
            fns,
            tns,
        };

        LPPerf(){
            trueSkip=trueNoSkip=falseSkip=falseNoSkip=total=Helper::Counter(0);
        }
        Helper::Counter getTrueSkip(){return trueSkip;}
        Helper::Counter getTrueNoSkip(){return trueNoSkip;}
        Helper::Counter getFalseSkip(){return falseSkip;}
        Helper::Counter getFalseNoSkip(){return falseNoSkip;}
        UInt64 getTotalCount(){return trueSkip.getCount()+trueNoSkip.getCount()+falseSkip.getCount()+falseNoSkip.getCount();}

        void inc(State state, int times=1){
            switch(state){
                case State::fs: falseSkip.increase(times); break;
                case State::ts: trueSkip.increase(times); break;
                case State::fns: falseNoSkip.increase(times); break;
                case State::tns: trueNoSkip.increase(times); break;
            }
            total.increase();
        }

        UInt64 getCount(State state){
            switch(state){
                case State::fs: return falseSkip.getCount(); break;
                case State::ts: return trueSkip.getCount(); break;
                case State::fns: return falseNoSkip.getCount(); break;
                case State::tns: return trueNoSkip.getCount(); break;
            }
        }

        double getRatio(State state){
            Helper::Counter *counter=nullptr;
            switch(state){
                case State::fs: counter=&falseSkip; break;
                case State::ts: counter=&trueSkip; break;
                case State::fns: counter=&falseNoSkip; break;
                case State::tns: counter=&trueNoSkip; break;
            }
            if(counter!=nullptr)
              return (double)counter->getCount()/(double)total.getCount();
            else
                exit(0);
        }
    };

    class LPHelper
    {
        public:
        static int lp_unlock;
        static std::unordered_map<IntPtr, LevelPredictor> tmpAllLevelLP;
        static int getLockStatus(){return lp_unlock;}
        static void lockenable(){lp_unlock=1;}
        static int isLockEnabled(){return lp_unlock;}
        static void addSkip(IntPtr pc, MemComponent::component_t level){
            tmpAllLevelLP[pc].addSkipLevel(level);
        }
        static void insert(IntPtr pc){
            tmpAllLevelLP.insert({pc, LevelPredictor()});
        }
        static void clearLPTable(){
            // copyTmpAllLevelLP.clear();
            // std::copy(tmpAllLevelLP.begin(), tmpAllLevelLP.end(), copyTmpAllLevelLP.begin());
            tmpAllLevelLP.clear();
        }
        static int  getTopPCcount(){
            return tmpAllLevelLP.size();
        }       
    };

    class PCStatHelper
    {
        typedef PCStat EpocPerformanceStat;
        typedef std::map<int, PCStat> LevelPCStat;
        MemComponent::component_t llc;
        int core;
        public:
        PCStatHelper(int core, MemComponent::component_t llc=MemComponent::component_t::LAST_LEVEL_CACHE){
            this->core=core;
            this->llc=llc;
            totalAccessPerEpoc=0;
            counter=0;
            lpEntryFound=0;
            lpaccess=Helper::Counter(0);
            for(int level=MemComponent::component_t::L1_DCACHE; level<= llc; level++){
                memLevels.push_back(static_cast<MemComponent::component_t>(level));
                perLevelAccuracy.insert({level,Helper::Counter(0)});
            }
            memLevels.push_back(MemComponent::component_t::DRAM);
            l1=l2=l3=Helper::Counter(0);
            howManyMatch=howManyCmp=Helper::Counter(0);
        }
        
        // total access
        Helper::Counter l1,l2,l3;

        void setLLC(MemComponent::component_t llc){this->llc=llc;}
        std::unordered_map<IntPtr, Helper::Counter> perEpocperPCStat;
        UInt64 totalAccessPerEpoc, lpEntryFound;

        //epoc based but reset for next epoc usage
        std::unordered_map<IntPtr, LevelPCStat> tmpAllLevelPCStat;//being reset
        // per level keep track of total access
        std::map<int, Helper::Counter> perLevelAccessCount, perLevelAccuracy;
        // per level keep track of uniq pc count
        std::map<int, std::unordered_set<IntPtr>> perLevelUniqPC;
        // per pc per level per epoc LP perfromance (missmatch between actual and prediction for all levels)
        std::unordered_map<IntPtr, std::vector<EpocPerformanceStat>> perPCperLevelperEpocLPPerf;
       
        std::vector<MemComponent::component_t> memLevels;
        // LP x-1, x performance comparison
        std::map<int, LPPerf> perLevelLPperf,accessTypeCount;

        Helper::Counter lpaccess;
        // epoc counter
        UInt64 counter;

        Helper::Counter howManyCmp, howManyMatch;

        void reset(UInt64 x){
            howManyCmp.reset();
            howManyMatch.reset();
            tmpAllLevelPCStat.clear();
            perPCperLevelperEpocLPPerf.clear();
            perEpocperPCStat.clear();
            totalAccessPerEpoc=0;
            lpEntryFound=0;
            perLevelAccessCount.clear();
            perLevelUniqPC.clear();
            counter=x;
            perLevelLPperf.clear();
            accessTypeCount.clear();
            
            for(auto comp: memLevels){
                perLevelLPperf[comp]=LPPerf();
                accessTypeCount[comp]=LPPerf();
                perLevelAccuracy[comp].reset();
            }
            lpaccess.reset();
        }

        void lockenable(){LPHelper::lockenable();}
        int isLockEnabled(){return LPHelper::isLockEnabled();}

        UInt64 getPerLevelUniqPCCount(int level){
            auto findLevel = perLevelUniqPC.find(level);
            if(findLevel!=perLevelUniqPC.end()){
                return perLevelUniqPC[level].size();
            }
            printf("\n\n_______________********______________getPerLevelUniqPCCount_\n\n");
            return 0;
        }

        UInt64 getPerLevelAccessCount(int level){
            auto findLevel = perLevelAccessCount.find(level);
            if(findLevel!=perLevelAccessCount.end())
                return findLevel->second.getCount();
            printf("\n\n_______________********______________getPerLevelAccessCount_\n\n");
            return 1;
        }

        UInt64 getThreshold(){
            return totalAccessPerEpoc/tmpAllLevelPCStat.size();
        }

        UInt64 getThresholdByLevel(int level){
            return getPerLevelAccessCount(level)/getPerLevelUniqPCCount(level);
        }

        UInt64 getTotalAccessInCurrEpoc(){return totalAccessPerEpoc;}

        void countTotalAccess(int level){
            if(level<=3)// counting only accesses at L1D 
                totalAccessPerEpoc++;
        }

        void countPerLevelAccess(int level, bool cache_hit)
        {
            // per level total access count
            if(perLevelAccessCount.find(level)!=perLevelAccessCount.end()){
                perLevelAccessCount[level].increase();
            }
            else perLevelAccessCount.insert({level, Helper::Counter(1)});
        }

        void countPerLevelUniqPC(int level, IntPtr pc){
            perLevelUniqPC[level].insert(pc);           
        }

        void addPerEpocPerPCinfo(IntPtr pc, int level){
            if(level>3)// implicitly only caculating traffic at L1D for picking top pc
                return;
            if(perEpocperPCStat.find(pc)!=perEpocperPCStat.end()){
                perEpocperPCStat[pc].increase();
            }
            else perEpocperPCStat[pc]=Helper::Counter(1);
        }

        bool LPPredictionVerifier(IntPtr pc, MemComponent::component_t actual_level, bool cache_hit){
            
            if(LPHelper::getLockStatus()==1){
                LPPredictionVerifier2(pc,actual_level,cache_hit);
            }
            return false;
        }

        bool LPPredictionVerifier2(IntPtr pc, MemComponent::component_t actual_level, bool cache_hit){
            
            if(LPHelper::getLockStatus()==1){
                // if not cache hit and not llc return else if at llc its cache miss or any level and hit then process further
                if(!cache_hit && actual_level!=llc){
                    if(LPHelper::tmpAllLevelLP.find(pc) != LPHelper::tmpAllLevelLP.end()){
                    }
                    return false;
                }
                if(LPHelper::tmpAllLevelLP.size()==0)
                    return false;
                // if prediction found then only do missmatch bookkepping
                LevelPredictor *prediction = new LevelPredictor();
                if(LPHelper::tmpAllLevelLP.find(pc) != LPHelper::tmpAllLevelLP.end()){
                    prediction = &LPHelper::tmpAllLevelLP[pc];
                }else return false;
                
                // these should be equal to sum of type access to verify inconsistent sum case between total LP access and type access sum
                lpaccess.increase();// wont be counted, if prediction exist but its not cache hit at all levels, suspecting that data not found at any cache levels and hence even after lp hit, as no cache hit happend, counter to count lp type access(fs,ts etc) is never called

                // create boolean representation of skip-noskip accesses for all level using LevelPredictor for actual event
                LevelPredictor lp = LevelPredictor();
                for(int i=MemComponent::component_t::L1_DCACHE;i<=llc;i++){
                    if(i==actual_level)
                        continue;
                    lp.addSkipLevel(static_cast<MemComponent::component_t>(i));
                }
                
                // insert if no pc found for bookkeeping of mismatch
                if(perPCperLevelperEpocLPPerf.find(pc)==perPCperLevelperEpocLPPerf.end())
                {
                    perPCperLevelperEpocLPPerf.insert({pc,std::vector<EpocPerformanceStat>(llc - MemComponent::component_t::L1_DCACHE+1, PCStat())});
                } 

                // observe how many of accesses goes for fs,fns,ts,tns per level.
                for(int i=MemComponent::component_t::L1_DCACHE; i<=llc; i++){
                    MemComponent::component_t comp = static_cast<MemComponent::component_t>(i);
                    bool actSkip = lp.canSkipLevel(comp);
                    bool predSkip = prediction->canSkipLevel(comp);

                    if(predSkip){// prediction was skip
                        if(actSkip){// actual is skip
                            accessTypeCount[comp].inc(LPPerf::State::ts);//benefit
                        }
                        else{
                            accessTypeCount[comp].inc(LPPerf::fs);//hazard
                        }
                    }
                    else{// prediction was no-skip
                        if(actSkip){// actual is skip
                            accessTypeCount[comp].inc(LPPerf::fns);//lost opp
                        }else{
                            accessTypeCount[comp].inc(LPPerf::tns);//benefit
                        }
                    } 
                    if(actSkip==0)
                        break;
                }

                bool predMatch = false;
                // per level check skip-noskip missmatch or match, keep count
                for(int i=MemComponent::component_t::L1_DCACHE;i<=llc;i++){
                    MemComponent::component_t comp = static_cast<MemComponent::component_t>(i);
                    bool actSkip = lp.canSkipLevel(comp);
                    bool predSkip = prediction->canSkipLevel(comp);

                    if(actSkip != predSkip){
                        perPCperLevelperEpocLPPerf[pc][i - MemComponent::component_t::L1_DCACHE].increaseMiss();// hazardous
                    }
                    else{
                        perPCperLevelperEpocLPPerf[pc][i - MemComponent::component_t::L1_DCACHE].increaseHit();
                    }
                    
                    perLevelAccuracy[i].increase();

                    if(actSkip==predSkip && actSkip==0)// actual level and predction 
                    {
                        predMatch=true;
                    }
                }
                
                if(predMatch){
                    howManyMatch.increase();
                }
            }
            
            return false;
        }

        void insert(std::unordered_map<IntPtr, LevelPCStat>& tmpAllLevelPCStat, int level, IntPtr pc, bool cache_hit){
            auto findPc = tmpAllLevelPCStat.find(pc);
            if(findPc!=tmpAllLevelPCStat.end()){
               
                auto findLevel=findPc->second.find(level);
                if(findLevel!=tmpAllLevelPCStat[pc].end()){
                    if(cache_hit)
                        findLevel->second.increaseHit();
                    else findLevel->second.increaseMiss();
                }
                else{
                    PCStat pcstat = PCStat(cache_hit);
                    tmpAllLevelPCStat[pc].insert({level, pcstat});
                }
            }
            else{
                LevelPCStat levelPCStat = LevelPCStat();
                PCStat pcstat = PCStat(cache_hit);
                levelPCStat.insert({level,pcstat});
                tmpAllLevelPCStat.insert({pc,levelPCStat});
            }
        }

        void insert2EpocStat(int level, IntPtr pc, bool cache_hit){
            countPerLevelAccess(level,cache_hit);
            countPerLevelUniqPC(level,pc);
            insert(tmpAllLevelPCStat, level,pc,cache_hit);            
        }

        void insertEntry(int level, IntPtr pc, bool cache_hit){
            if(level<=2)
                return;
            addPerEpocPerPCinfo(pc, level);
            countTotalAccess(level);
            insert2EpocStat(level,pc,cache_hit);
        }

        bool learnFromPrevEpoc(IntPtr pc, MemComponent::component_t level);

        void updateLPTable();

        // comparing decision from x-1 and how effective those at x
        void processEndPerformanceAnalysis(IntPtr pc, LevelPredictor levelPred);
        bool interpretMMratio(double ratio);
        int getTotalPCCount(){return tmpAllLevelPCStat.size();}
        // per level count is non-redundant, return total sum
        UInt64 getTotalLPAccessCount();
        // lp lookup
        void LPLookup(IntPtr pc);
        // LOGGERS
        // per level unique pc count
        void logPerLevelPCCount();
        // collected at end of x and log at end as well// consider all levels
        void logPerEpocSkiporNotSkipStatus(std::vector<Helper::Message>&msg, IntPtr pc);
        // top pc vs total pc count
        void logTopVsTotalPC();
        // per level LP access vs total access
        void logPerLevelLPVsTotalLPAccess();
        void logLPTopPCTotalAccessCount();
        void logLPvsTypeAccess();
        // init logging
        void logInit();

        // will return epoc based pc stat to log, as well it adds skipable levels;
        void processEpocEndComputation(IntPtr pc, std::unordered_map<IntPtr, LevelPCStat>& mp, UInt64 counter)
        {
            LPHelper::insert(pc);

            // insert into LP table per level skip status               
            for(auto uord: mp[pc])
            {
                PCStat pcStat= uord.second;
                UInt64 total = pcStat.getHitCount() + pcStat.getMissCount();
                Helper::Message msg = Helper::Message(-1,-1, static_cast<MemComponent::component_t>(uord.first), total,
                 pcStat.getMissCount());
                
                // for per level filterrring
                UInt64 thresh = getThresholdByLevel(msg.getLevel());
                UInt64 pccount = uord.second.getTotalCount();
                
                if(msg.getMissRatio()>0.5111){// && learnFromPrevEpoc(pc,msg.getLevel())
                    LPHelper::addSkip(pc, msg.getLevel());
                    msg.addLevelSkip();
                }

            }
        }
    };

}
#endif