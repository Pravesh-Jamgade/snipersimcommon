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
            trueSkip=trueNoSkip=falseSkip=falseNoSkip=total=Helper::Counter();
        }
        Helper::Counter getTrueSkip(){return trueSkip;}
        Helper::Counter getTrueNoSkip(){return trueNoSkip;}
        Helper::Counter getFalseSkip(){return falseSkip;}
        Helper::Counter getFalseNoSkip(){return falseNoSkip;}

        void inc(State state){
            switch(state){
                case State::fs: falseSkip.increase(); break;
                case State::ts: trueSkip.increase(); break;
                case State::fns: falseNoSkip.increase(); break;
                case State::tns: trueNoSkip.increase(); break;
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
        static std::unordered_map<IntPtr, LevelPredictor> copyTmpAllLevelLP;
        static int getLockStatus(){return lp_unlock;}
        static void lockenable(){lp_unlock=1;}
        static int isLockEnabled(){return lp_unlock;}
        static void insert(IntPtr pc, MemComponent::component_t level){
            if(tmpAllLevelLP.find(pc)!=tmpAllLevelLP.end())
                tmpAllLevelLP[pc].addSkipLevel(level);
            else tmpAllLevelLP.insert({pc, LevelPredictor(level)});
        }
        static void clearLPTable(){
            copyTmpAllLevelLP.erase(copyTmpAllLevelLP.begin(), copyTmpAllLevelLP.end());
            copyTmpAllLevelLP=tmpAllLevelLP;
            tmpAllLevelLP.erase(tmpAllLevelLP.begin(), tmpAllLevelLP.end());} 

        static int  getTopPCcount(){
            return tmpAllLevelLP.size();
        }       
    };

    class PCStatHelper
    {
        typedef PCStat EpocPerformanceStat;
        typedef std::map<int, PCStat> LevelPCStat;
        MemComponent::component_t llc;
        public:
        PCStatHelper(MemComponent::component_t llc=MemComponent::component_t::LAST_LEVEL_CACHE){
            lpPerformance = std::unique_ptr<EpocPerformanceStat>(new EpocPerformanceStat());
            localPerformance=std::unique_ptr<EpocPerformanceStat>(new EpocPerformanceStat());
            allMemLocalPerformance=std::vector<EpocPerformanceStat>(llc - MemComponent::component_t::L1_DCACHE + 1);
            lpskipPerformance=std::vector<EpocPerformanceStat>(llc - MemComponent::component_t::L1_DCACHE + 1);
            this->llc=llc;
            totalAccessPerEpoc=0;
            counter=0;
            precisionLPHitActualLevel=0;
        }
        
        void setLLC(MemComponent::component_t llc){this->llc=llc;}
        std::unordered_map<IntPtr, Helper::Counter> perEpocperPCStat;
        UInt64 totalAccessPerEpoc;

        //epoc based but reset for next epoc usage
        std::unordered_map<IntPtr, LevelPCStat> tmpAllLevelPCStat;//being reset
        //epoc based but cumulative
        std::unordered_map<IntPtr, LevelPCStat> globalAllLevelPCStat;//not reset
        // per epoc hit,miss information (to understand how good/bad epoc did)
        std::unique_ptr<EpocPerformanceStat> localPerformance;
        // per epoc hit,miss information for LP lookup (to understand how useful LP table was in next incomming epoc)
        std::unique_ptr<EpocPerformanceStat> lpPerformance;
        // per epoc per level hit,miss information
        std::vector<EpocPerformanceStat> allMemLocalPerformance;
        // per epoc per level skip,noskip information for LP (to understand how levels are being used by skip or noskip in an epoc)
        std::vector<EpocPerformanceStat> lpskipPerformance;
        
        // per level keep track of total access
        std::map<int, Helper::Counter> perLevelAccessCount, perLevelLPAccessCount;
        // per level keep track of uniq pc count
        std::map<int, std::unordered_set<IntPtr>> perLevelUniqPC;
        // per pc per level per epoc LP perfromance (missmatch between actual and prediction for all levels)
        std::unordered_map<IntPtr, std::vector<EpocPerformanceStat>> perPCperLevelperEpocLPPerf;
        // per pc per level per epoc hit miss performance
        std::unordered_map<IntPtr, std::vector<EpocPerformanceStat>> perPCperLevelPerEpocHitMissPerf;

        std::map<MemComponent::component_t, Helper::Counter> perLevelSkip, perLevelNoSkip;
        std::map<MemComponent::component_t, Helper::Counter> perLevelSkipWhileEpoc, perLevelNoSkipWhileEpoc;
        
        // LP x-1, x performance comparison
        std::map<int, LPPerf> perLevelLPperf;
        UInt64 precisionLPHitActualLevel;
        // epoc counter
        UInt64 counter;

        void reset(UInt64 x){
            tmpAllLevelPCStat.erase(tmpAllLevelPCStat.begin(), tmpAllLevelPCStat.end());
            localPerformance.reset(new EpocPerformanceStat());
            allMemLocalPerformance.erase(allMemLocalPerformance.begin(), allMemLocalPerformance.end());
            lpPerformance.reset(new EpocPerformanceStat());
            lpskipPerformance.erase(lpskipPerformance.begin(),lpskipPerformance.end());
            perPCperLevelperEpocLPPerf.erase(perPCperLevelperEpocLPPerf.begin(), perPCperLevelperEpocLPPerf.end());
            perPCperLevelPerEpocHitMissPerf.erase(perPCperLevelPerEpocHitMissPerf.begin(), perPCperLevelPerEpocHitMissPerf.end());
            perEpocperPCStat.erase(perEpocperPCStat.begin(), perEpocperPCStat.end());
            totalAccessPerEpoc=0;
            precisionLPHitActualLevel=0;
            perLevelAccessCount.erase(perLevelAccessCount.begin(), perLevelAccessCount.end());
            perLevelLPAccessCount.erase(perLevelLPAccessCount.begin(), perLevelLPAccessCount.end());
            perLevelUniqPC.erase(perLevelUniqPC.begin(), perLevelUniqPC.end());
            counter=x;
            perLevelSkip.erase(perLevelSkip.begin(),perLevelSkip.end());
            perLevelNoSkip.erase(perLevelNoSkip.begin(),perLevelNoSkip.end());
            perLevelSkipWhileEpoc.erase(perLevelSkipWhileEpoc.begin(),perLevelSkipWhileEpoc.end());
            perLevelNoSkipWhileEpoc.erase(perLevelNoSkipWhileEpoc.begin(),perLevelNoSkipWhileEpoc.end());

            for(int i=MemComponent::component_t::L1_DCACHE; i<=llc; i++){
                MemComponent::component_t comp = static_cast<MemComponent::component_t>(i);
                perLevelNoSkip[comp]=Helper::Counter(0);
                perLevelSkip[comp]=Helper::Counter(0);
                perLevelNoSkipWhileEpoc[comp]=Helper::Counter(0);
                perLevelSkipWhileEpoc[comp]=Helper::Counter(0);
                perLevelLPperf[comp]=LPPerf();
            }
            perLevelLPperf.erase(perLevelLPperf.begin(), perLevelLPperf.end());
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
        double getLPLocalEpocHitRatio(){return 1-lpPerformance->getMissRatio();}
        double getLPLocalEpocMissRatio(){return lpPerformance->getMissRatio();}
        double getLocalEpocHitRatio(){return 1-localPerformance->getMissRatio();}
        double getLocalEpocMissRatio(){return localPerformance->getMissRatio();}

        void countLevelPerformance(int level, bool cache_hit){
            if(cache_hit){
                allMemLocalPerformance[level - MemComponent::component_t::L1_DCACHE].increaseHit();
            }
            else{
                allMemLocalPerformance[level - MemComponent::component_t::L1_DCACHE].increaseMiss();
            }
        }

        void countPerformance(bool cache_hit){
            if(cache_hit){
                localPerformance->increaseHit();
            }
            else{
                localPerformance->increaseMiss();
            }
        }

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

        std::vector<MemComponent::component_t> LPPrediction(IntPtr pc){
            std::vector<MemComponent::component_t> predicted_levels;
            // LP prediction
            if(LPHelper::getLockStatus()==1)
            {
                auto ptr = LPHelper::tmpAllLevelLP.find(pc);
                if(ptr!=LPHelper::tmpAllLevelLP.end()){
                    lpPerformance->increaseHit();
                    for(int i=MemComponent::component_t::L1_DCACHE;i<= llc; i++){// (5-3+1)=3 level's of cache
                        if(ptr->second.canSkipLevel(static_cast<MemComponent::component_t>(i) )){
                            predicted_levels.push_back(static_cast<MemComponent::component_t>(i));
                            lpskipPerformance[i-MemComponent::component_t::L1_DCACHE].increaseHit();
                        }
                        else lpskipPerformance[i-MemComponent::component_t::L1_DCACHE].increaseMiss();
                    }
                }else lpPerformance->increaseMiss();
            }
            return predicted_levels;
        }

        void addPerEpocPerPCinfo(IntPtr pc, int level){
            if(level>3)
                return;
            if(perEpocperPCStat.find(pc)!=perEpocperPCStat.end()){
                perEpocperPCStat[pc].increase();
            }
            else perEpocperPCStat[pc]=Helper::Counter(1);
        }

        bool LPPredictionVerifier(IntPtr pc, MemComponent::component_t actual_level){
            
            if(LPHelper::getLockStatus()==1){
                bool lookupSuccess = LPPredictionVerifier2(pc,actual_level);
                if(lookupSuccess)// we found pc in LPtable 
                {
                    // incraease lookup count per level for LP successful access
                    auto findLevel = perLevelLPAccessCount.find(actual_level);
                    if(findLevel!=perLevelLPAccessCount.end())
                        perLevelLPAccessCount[actual_level].increase();
                    else perLevelLPAccessCount[actual_level]=Helper::Counter(1);
                }

            }
            return false;
        }

        bool LPPredictionVerifier2(IntPtr pc, MemComponent::component_t actual_level){
            
            if(LPHelper::getLockStatus()==1){
                // if prediction found then only do missmatch bookkepping
                LevelPredictor *prediction = new LevelPredictor();
                if(LPHelper::tmpAllLevelLP.find(pc) != LPHelper::tmpAllLevelLP.end()){
                    prediction = &LPHelper::tmpAllLevelLP[pc];
                }else return false;
                
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

                // per level check skip-noskip missmatch or match, keep count
                for(int i=MemComponent::component_t::L1_DCACHE;i<=llc;i++){
                    MemComponent::component_t comp = static_cast<MemComponent::component_t>(i);
                    bool actSkip = lp.canSkipLevel(comp);
                    bool predSkip = prediction->canSkipLevel(comp);

                    // if(comp == llc && predSkip==1){
                    //     _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TEMP, "%ld\n", counter);
                    // }

                    // _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogState::DISABLE, Log::LogDst::DEBUG_PER_EPOC_N_SKIP, "%ld,%d,%d\n", counter,predSkip,actSkip);
                    if(predSkip)
                        perLevelSkipWhileEpoc[comp].increase();
                    else 
                        perLevelNoSkipWhileEpoc[comp].increase();

                    if(actSkip != predSkip){
                        perPCperLevelperEpocLPPerf[pc][i - MemComponent::component_t::L1_DCACHE].increaseMiss();// hazardous
                    }
                    else{
                        perPCperLevelperEpocLPPerf[pc][i - MemComponent::component_t::L1_DCACHE].increaseHit();
                    }

                    if(actSkip == 0 && actSkip==predSkip)// actual level and predction for that level is both no-skip
                    {
                        precisionLPHitActualLevel++;
                    }

                    if(actSkip==0){
                        // lets not count miss-match further
                        return true;
                    }
                }
            }
            
            return true;
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

        void insertToBoth(int level, IntPtr pc, bool cache_hit){
            // if(level<=2){
            //     return;
            // }
            countLevelPerformance(level,cache_hit);
            countPerLevelAccess(level,cache_hit);
            countPerLevelUniqPC(level,pc);
            insert(tmpAllLevelPCStat, level,pc,cache_hit);            
            insert(globalAllLevelPCStat, level,pc,cache_hit);            
            // insertToBoth(level-1, pc, false);
        }

        void insertEntry(int level, IntPtr pc, bool cache_hit){
            if(level<=2)
                return;
            addPerEpocPerPCinfo(pc, level);
            countTotalAccess(level);
            countPerformance(cache_hit);
            insertToBoth(level,pc,cache_hit);
        }

        bool learnFromPrevEpoc(IntPtr pc, MemComponent::component_t level);

        void updateLPTable();


        // comparing decision from x-1 and how effective those at x
        void processEndPerformanceAnalysis(IntPtr pc);
        bool interpretMMratio(double ratio);
        int getTotalPCCount(){return tmpAllLevelPCStat.size();}
        // per level count is non-redundant, return total sum
        UInt64 getTotalLPAccessCount();
        // LOGGERS

        // log per epoc total accesses, threshold 
        void logPerEpocTotalAccess();
        // log all pc from epoc and their total count
        void logPerPCTotalCountPerEpoc();
        // log per level access per epoc
        void logPerLevelAccessCountPerEpoc();
        //log per pc per level per epoc access
        void logPerPCPerLevelAccessCountperEpoc();
        // dump LP table
        void logLPTablePerEpoc();
        // log per pc per level miss ratio for epoc
        void logPerPCPerLevelMissratioPerEpoc();
        // per level threshold
        void logPerLevelPerEpocAccessThreshold();
        // per level unique pc count
        void logPerLevelPCCount();
        // collected at end of x and log at end as well// consider all levels
        void logPerEpocSkiporNotSkipStatus(std::vector<Helper::Message>&msg, IntPtr pc);
        // collected during x epoc and logged at end of x epoc// donot consider to count the level above actual level where data found
        void logPerEpocSkiporNotskipWhileEpoc();
        void logLPPerfPerLevel();
        // top pc vs total pc count
        void logTopVsTotalPC();
        // per level LP access vs total access
        void logPerLevelLPVsTotalLPAccess();
        // total LP access vs total precise prediction overlapping with actual level
        void logLPTotalVsPreciseHitCount();
        // init logging
        void logInit();

        // will return epoc based pc stat to log, as well it adds skipable levels;
        std::vector<Helper::Message> processEpocEndComputation(IntPtr pc, std::unordered_map<IntPtr, LevelPCStat>& mp, UInt64 counter)
        {
            std::vector<Helper::Message> allMsg;
            
            // if per pc access is less than threshold then do not consider to add it into LP table as avg aacesses are less
            if( perEpocperPCStat[pc].getCount() < getThreshold())
                return allMsg;
            

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
                
                if(msg.getMissRatio()>0.4 && learnFromPrevEpoc(pc,msg.getLevel())){//learnFromPrevEpoc(pc, msg.getLevel())
                    LPHelper::insert(pc, msg.getLevel());
                    msg.addLevelSkip();
                    perLevelSkip[msg.getLevel()].increase();
                }
                else
                {
                    perLevelNoSkip[msg.getLevel()].increase();
                }

                allMsg.push_back(msg);// can be used for logging
            }
            return allMsg;
        }
    };

}
#endif