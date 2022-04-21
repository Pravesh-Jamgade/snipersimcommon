#ifndef PCPRED_H
#define PCPRED_H

#include<iostream>
#include<vector>
#include<unordered_map>
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

    class EpocStat
    {
        Helper::Counter trueSkipLoss, trueSkipOppo, falseSkip, noPrediction, totalAccess; 
        IntPtr getTotalAccess(){return totalAccess.getCount();}
        IntPtr getNoPrediction(){return noPrediction.getCount();}
        IntPtr getFalseSkip(){return falseSkip.getCount();}
        IntPtr getTrueSkipOppo(){return trueSkipOppo.getCount();}
        IntPtr getTrueSkipLoss(){return trueSkipLoss.getCount();}
        IntPtr getTotalTrueSkip(){ return getTrueSkipOppo()+getTrueSkipLoss();}
        public:
        EpocStat(){
            trueSkipLoss=trueSkipOppo=falseSkip=noPrediction=totalAccess=Helper::Counter();
        }

        void incTrueSkipLossCounter(){ trueSkipLoss.increase();}
        void incTrueSkipOppoCounter(){ trueSkipOppo.increase();}
        void incFalseSkipCounter(){ falseSkip.increase();}
        void incNoPredictionCounter(){ noPrediction.increase();}
        void incTotalAccessCounter(){ totalAccess.increase();}

        void reset(){trueSkipOppo.reset(), trueSkipLoss.reset(), falseSkip.reset(), noPrediction.reset(), totalAccess.reset();}
        double getNoPredRatio(){ return (double)getNoPrediction()/(double)getTotalAccess();}
        double getTrueSkipRatio(){ return (double)(getTrueSkipLoss() +  getTrueSkipOppo()) / (double)getTotalAccess() ;}
        double getFalseSkipRatio(){return (double)getFalseSkip()/(double)getTotalAccess();}
        double getTrueSkipLossRatio(){return (double)getTrueSkipLoss()/(double)getTotalAccess();}
        double getTrueSkipOppoRatio(){return (double)getTrueSkipOppo()/(double)getTotalAccess();}
    };

    class LPHelper
    {
        public:
        static int lp_unlock;
        static std::unordered_map<IntPtr, LevelPredictor> tmpAllLevelLP;
        static int getLockStatus(){return lp_unlock;}
        static void lockenable(){lp_unlock=1;}
        static int isLockEnabled(){return lp_unlock;}
        static void insert(IntPtr pc, MemComponent::component_t level){
            if(tmpAllLevelLP.find(pc)!=tmpAllLevelLP.end())
                tmpAllLevelLP[pc].addSkipLevel(level);
            else tmpAllLevelLP.insert({pc, LevelPredictor(level)});
        }        
    };

    class PCStatHelper
    {
        typedef PCStat EpocPerformanceStat;
        typedef std::map<int, PCStat> LevelPCStat;
        MemComponent::component_t llc;
        public:
        PCStatHelper(MemComponent::component_t llc=MemComponent::component_t::LAST_LEVEL_CACHE){
            localEpocStat = new EpocStat();
            lpPerformance = std::unique_ptr<EpocPerformanceStat>(new EpocPerformanceStat());
            localPerformance=std::unique_ptr<EpocPerformanceStat>(new EpocPerformanceStat());
            allMemLocalPerformance=std::vector<EpocPerformanceStat>(llc - MemComponent::component_t::L1_DCACHE + 1);
            lpskipPerformance=std::vector<EpocPerformanceStat>(llc - MemComponent::component_t::L1_DCACHE + 1);
            this->llc=llc;
        }
        
        void setLLC(MemComponent::component_t llc){this->llc=llc;}
        std::unordered_map<IntPtr, Helper::Counter> perEpocperPCStat;
        UInt64 totalAccessPerEpoc;

        //epoc based but reset for next epoc usage
        std::unordered_map<IntPtr, LevelPCStat> tmpAllLevelPCStat;//being reset
        //epoc based but cumulative
        std::unordered_map<IntPtr, LevelPCStat> globalAllLevelPCStat;//not reset
        // LP prediciton accuracy by types of prediction
        EpocStat *localEpocStat;
        // per epoc hit,miss information (to understand how good/bad epoc did)
        std::unique_ptr<EpocPerformanceStat> localPerformance;
        // per epoc hit,miss information for LP lookup (to understand how useful LP table was in next incomming epoc)
        std::unique_ptr<EpocPerformanceStat> lpPerformance;
        // per epoc per level hit,miss information
        std::vector<EpocPerformanceStat> allMemLocalPerformance;
        // per epoc per level skip,noskip information for LP (to understand how levels are being used by skip or noskip in an epoc)
        std::vector<EpocPerformanceStat> lpskipPerformance;
        
        // per level keep track of total access
        std::map<int, Helper::Counter> perLevelAccessCount;
        // per level keep track of uniq pc count
        std::map<int, Helper::Counter> perLevelUniqPC;
        // per pc per level per epoc LP perfromance (missmatch between actual and prediction for all levels)
        std::unordered_map<IntPtr, std::vector<EpocPerformanceStat>> perPCperLevelperEpocLPPerf;
        // per pc per level per epoc hit miss performance
        std::unordered_map<IntPtr, std::vector<EpocPerformanceStat>> perPCperLevelPerEpocHitMissPerf;

        void reset(){
            tmpAllLevelPCStat.erase(tmpAllLevelPCStat.begin(), tmpAllLevelPCStat.end());
            localEpocStat->reset();
            localPerformance.reset(new EpocPerformanceStat());
            allMemLocalPerformance.erase(allMemLocalPerformance.begin(), allMemLocalPerformance.end());
            lpPerformance.reset(new EpocPerformanceStat());
            lpskipPerformance.erase(lpskipPerformance.begin(),lpskipPerformance.end());
            perPCperLevelperEpocLPPerf.erase(perPCperLevelperEpocLPPerf.begin(), perPCperLevelperEpocLPPerf.end());
            perPCperLevelPerEpocHitMissPerf.erase(perPCperLevelPerEpocHitMissPerf.begin(), perPCperLevelPerEpocHitMissPerf.end());
            perEpocperPCStat.erase(perEpocperPCStat.begin(), perEpocperPCStat.end());
            totalAccessPerEpoc=0;
            perLevelAccessCount.erase(perLevelAccessCount.begin(), perLevelAccessCount.end());
            perLevelUniqPC.erase(perLevelUniqPC.begin(), perLevelUniqPC.end());
            
        }

        void lockenable(){LPHelper::lockenable();}
        int isLockEnabled(){return LPHelper::isLockEnabled();}

        UInt64 getPerLevelUniqPCCount(int level){
            auto findLevel = perLevelUniqPC.find(level);
            if(findLevel!=perLevelUniqPC.end()){
                return perLevelUniqPC[level].getCount();
            }
            printf("\n\n_______________********______________getPerLevelUniqPCCount_\n\n");
            return 1;
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

        void countTotalAccess(){
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

        void countPerLevelUniqPC(int level, int pc){
            auto findLevel = perLevelUniqPC.find(level);
            if(findLevel!=perLevelUniqPC.end()){
                findLevel->second.increase();
            }
            else{
                perLevelUniqPC.insert({level, Helper::Counter(1)});
            }
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

        void addPerEpocPerPCinfo(IntPtr pc){
            if(perEpocperPCStat.find(pc)!=perEpocperPCStat.end()){
                perEpocperPCStat[pc].increase();
            }
            else perEpocperPCStat[pc]=Helper::Counter(1);
        }

        bool LPPredictionVerifier(IntPtr pc, MemComponent::component_t actual_level){
            
            if(LPHelper::getLockStatus()==1){
                LPPredictionVerifier2(pc,actual_level);
                // total count
                localEpocStat->incTotalAccessCounter();

                std::vector<MemComponent::component_t> vec= LPPrediction(pc);
                if(vec.size()==0)
                {
                    localEpocStat->incNoPredictionCounter();
                    return false;
                }

                if(std::find(vec.begin(),vec.end(), actual_level)!=vec.end())
                {
                    localEpocStat->incFalseSkipCounter();
                    return false;
                }
                
                vec.push_back(actual_level);
                std::sort(vec.begin(), vec.end());
                auto pos = std::find(vec.begin(), vec.end(), actual_level);
                int index = vec.begin() - pos;
                if(index == 0){
                    // true skip + loss
                    localEpocStat->incTrueSkipLossCounter();
                }
                else{
                    // true skip + opportunity
                    localEpocStat->incTrueSkipOppoCounter();
                }
            }
            return false;
        }

        bool LPPredictionVerifier2(IntPtr pc, MemComponent::component_t actual_level){
            
            if(LPHelper::getLockStatus()==1){
                LevelPredictor *prediction = new LevelPredictor();
                if(LPHelper::tmpAllLevelLP.find(pc) != LPHelper::tmpAllLevelLP.end()){
                    prediction = &LPHelper::tmpAllLevelLP[pc];
                }else return false;

                LevelPredictor lp = LevelPredictor();
                for(int i=MemComponent::component_t::L1_DCACHE;i<=llc;i++){
                    if(i==actual_level)
                        continue;
                    lp.addSkipLevel(static_cast<MemComponent::component_t>(i));
                }
                
                if(perPCperLevelperEpocLPPerf.find(pc)==perPCperLevelperEpocLPPerf.end())
                {
                    perPCperLevelperEpocLPPerf.insert({pc,std::vector<EpocPerformanceStat>(llc - MemComponent::component_t::L1_DCACHE+1, PCStat())});
                } 
            
                for(int i=MemComponent::component_t::L1_DCACHE;i<=llc;i++){
                    if(lp.canSkipLevel(static_cast<MemComponent::component_t>(i)) != 
                            prediction->canSkipLevel(static_cast<MemComponent::component_t>(i))){
                        perPCperLevelperEpocLPPerf[pc][i - MemComponent::component_t::L1_DCACHE].increaseMiss();// hazardous
                    }
                    else{
                        perPCperLevelperEpocLPPerf[pc][i - MemComponent::component_t::L1_DCACHE].increaseHit();
                    }
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
            addPerEpocPerPCinfo(pc);
            countTotalAccess();
            countPerformance(cache_hit);
            insertToBoth(level,pc,cache_hit);
        }

        // will return epoc based pc stat to log, as well it adds skipable levels;
        std::vector<Helper::Message> processEpocEndComputation(IntPtr pc, std::unordered_map<IntPtr, LevelPCStat>& mp)
        {
            std::vector<Helper::Message> allMsg;
            bool firstRatio=true; 
            double currRatio;
            UInt64 prevMisses;

            // // if per pc access is less than threshold then do not consider to add it into LP table as avg aacesses are less
            // if( perEpocperPCStat[pc].getCount() < getThreshold())
            //     return allMsg;

            // insert into LP table per level skip status               
            for(auto uord: mp[pc])
            {
                PCStat pcStat= uord.second;
                UInt64 total = pcStat.getHitCount() + pcStat.getMissCount();
                Helper::Message msg = Helper::Message(-1,-1, static_cast<MemComponent::component_t>(uord.first), total,
                 pcStat.getMissCount());
                
                if(firstRatio){
                    currRatio=(double)msg.gettotalMiss()/(double)msg.gettotalAccess();
                    firstRatio=false;
                }
                else{
                    currRatio=(double)msg.gettotalMiss()/(double)prevMisses;
                } 

                prevMisses=msg.gettotalMiss();

                // for per level filterrring
                UInt64 thresh = getThresholdByLevel(msg.getLevel());
                UInt64 pccount = uord.second.getTotalCount();

                if(pccount < thresh){
                    LPHelper::insert(pc, msg.getLevel());
                    msg.addLevelSkip();
                }

                allMsg.push_back(msg);// can be used for logging
            }
            return allMsg;
        }
    };

}
#endif