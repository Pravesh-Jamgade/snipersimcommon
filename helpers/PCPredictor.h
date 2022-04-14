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
        void increaseMiss(){miss.increase();}
        void increaseHit(){hit.increase();}
        UInt64 getHitCount(){return hit.getCount();}
        UInt64 getMissCount(){return miss.getCount();}
        double getMissRatio(){return (double)miss.getCount()/(double)(miss.getCount()+hit.getCount()); }
        double getMissHitRate(){return (double)miss.getCount()/(double)(hit.getCount()); }
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

    class PCStatHelper
    {
        typedef PCStat EpocPerformanceStat;
        typedef std::map<int, PCStat> LevelPCStat;
        MemComponent::component_t llc;
        public:
        PCStatHelper(MemComponent::component_t llc){
            lp_unlock=2;
            globalEpocStat = new EpocStat();
            localEpocStat = new EpocStat();
            lpPerformance = std::unique_ptr<EpocPerformanceStat>(new EpocPerformanceStat());
            localPerformance=std::unique_ptr<EpocPerformanceStat>(new EpocPerformanceStat());
            globalPerformance=std::unique_ptr<EpocPerformanceStat>(new EpocPerformanceStat());
            allMemLocalPerformance=std::vector<EpocPerformanceStat>(llc - MemComponent::component_t::L1_DCACHE + 1);
            allMemGlobalPerformance=std::vector<EpocPerformanceStat>(llc - MemComponent::component_t::L1_DCACHE + 1);
            lpskipPerformance=std::vector<EpocPerformanceStat>(llc - MemComponent::component_t::L1_DCACHE + 1);
            this->llc=llc;
        }
        
        //epoc based but reset for next epoc usage
        std::unordered_map<IntPtr, LevelPCStat> tmpAllLevelPCStat;//being reset
        std::unordered_map<IntPtr, LevelPredictor> tmpAllLevelLP;//not reset

        //epoc based but cumulative
        std::unordered_map<IntPtr, LevelPCStat> globalAllLevelPCStat;//not reset
        std::unordered_map<IntPtr, LevelPredictor> globalAllLevelLP;//not reset
        
        EpocStat *globalEpocStat, *localEpocStat;// LP prediction accuracy
        std::unique_ptr<EpocPerformanceStat> localPerformance, globalPerformance;// hit/miss 
        std::unique_ptr<EpocPerformanceStat> lpPerformance;//LP prediction accuracy on local 
        std::vector<EpocPerformanceStat> allMemLocalPerformance, allMemGlobalPerformance, lpskipPerformance;// mem level wise m/h performance on local and global
        
        // LP in-use after debugEpoc epoc
        int lp_unlock;

        void reset(){
            tmpAllLevelPCStat.erase(tmpAllLevelPCStat.begin(), tmpAllLevelPCStat.end());
            localEpocStat->reset();
            localPerformance.reset(new EpocPerformanceStat());
            allMemLocalPerformance.erase(allMemLocalPerformance.begin(), allMemLocalPerformance.end());
            lpPerformance.reset(new EpocPerformanceStat());
            lpskipPerformance.erase(lpskipPerformance.begin(),lpskipPerformance.end());
        }

        void lockenable(){lp_unlock=1;}
        int isLockEnabled(){return lp_unlock;}
        
        double getLPLocalEpocHitRatio(){return 1-localPerformance->getMissRatio();}
        double getLPLocalEpocMissRatio(){return localPerformance->getMissRatio();}
        double getLocalEpocHitRatio(){return 1-localPerformance->getMissRatio();}
        double getLocalEpocMissRatio(){return localPerformance->getMissRatio();}
        double getGlobalEpocHitRatio(){return 1-globalPerformance->getMissRatio();}
        double getGlobalEpocMissRatio(){return globalPerformance->getMissRatio();}

        void countLevelPerformance(int level, bool cache_hit){
            if(cache_hit){
                allMemGlobalPerformance[level - MemComponent::component_t::L1_DCACHE].increaseHit();
                allMemLocalPerformance[level - MemComponent::component_t::L1_DCACHE].increaseHit();
            }
            else{
                allMemGlobalPerformance[level - MemComponent::component_t::L1_DCACHE].increaseMiss();
                allMemLocalPerformance[level - MemComponent::component_t::L1_DCACHE].increaseMiss();
            }
        }

        void countPerformance(bool cache_hit){
            if(cache_hit){
                localPerformance->increaseHit();
                globalPerformance->increaseHit();
            }
            else{
                localPerformance->increaseMiss();
                globalPerformance->increaseMiss();
            }
            
        }

        std::vector<MemComponent::component_t> LPPrediction(IntPtr pc){
            std::vector<MemComponent::component_t> predicted_levels;
            // LP prediction
            if(lp_unlock==1)
            {
                auto ptr = tmpAllLevelLP.find(pc);
                if(ptr!=tmpAllLevelLP.end()){
                    lpPerformance->increaseHit();
                    //TODO: remove these constant values
                    for(int i=MemComponent::component_t::L1_DCACHE;i<= llc; i++){// (5-3+1)=3 level's of cache
                        if(ptr->second.canSkipLevel(static_cast<MemComponent::component_t>(i) )){
                            predicted_levels.push_back(static_cast<MemComponent::component_t>(i));
                            lpskipPerformance[i-MemComponent::component_t::L1_DCACHE].getHitCount();
                        }
                        else lpskipPerformance[i-MemComponent::component_t::L1_DCACHE].getMissCount();
                    }
                }else lpPerformance->increaseMiss();
            }
            return predicted_levels;
        }

        bool LPPredictionVerifier(IntPtr pc, MemComponent::component_t actual_level){
            
            if(lp_unlock==1){
                // total count
                localEpocStat->incTotalAccessCounter();
                globalEpocStat->incTotalAccessCounter();

                _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_Prediction_MATCH, "%ld,", pc);

                std::vector<MemComponent::component_t> vec= LPPrediction(pc);
                if(vec.size()==0)
                {
                    localEpocStat->incNoPredictionCounter();
                    globalEpocStat->incNoPredictionCounter();
                    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_Prediction_MATCH, "No Predction\n");
                    return false;
                }

                if(std::find(vec.begin(),vec.end(), actual_level)!=vec.end())
                {
                    localEpocStat->incFalseSkipCounter();
                    globalEpocStat->incFalseSkipCounter();
                    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_Prediction_MATCH, "False Skip\n");
                    return false;
                }
                
                vec.push_back(actual_level);
                std::sort(vec.begin(), vec.end());
                auto pos = std::find(vec.begin(), vec.end(), actual_level);
                int index = vec.begin() - pos;
                if(index == 0){
                    // true skip + loss
                    localEpocStat->incTrueSkipLossCounter();
                    globalEpocStat->incTrueSkipLossCounter();
                }
                else{
                    // true skip + opportunity
                    localEpocStat->incTrueSkipOppoCounter();
                    globalEpocStat->incTrueSkipOppoCounter();
                }

                _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_Prediction_MATCH, "True Skip\n");
                
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
                    tmpAllLevelPCStat[pc].insert({level,PCStat()});
                }
            }
            else{
                LevelPCStat levelPCStat;
                levelPCStat.insert({level,PCStat()});
                tmpAllLevelPCStat.insert({pc,levelPCStat});
            }
        }

        void insertToBoth(int level, IntPtr pc, bool cache_hit){
            if(level<=2){
                return;
            }
            countLevelPerformance(level,cache_hit);
            insert(tmpAllLevelPCStat, level,pc,cache_hit);            
            insert(globalAllLevelPCStat, level,pc,cache_hit);            
            insertToBoth(level-1, pc, false);
        }

        void insertEntry(int level, IntPtr pc, bool cache_hit){
            if(level<2)
                return;
            countPerformance(cache_hit);
            insertToBoth(level,pc,cache_hit);
        }

        // will return epoc based pc stat to log, as well it adds skipable levels;
        std::vector<Helper::Message> processEpocEndComputation(IntPtr pc, std::unordered_map<IntPtr, LevelPCStat>& mp){
            std::vector<Helper::Message> allMsg;
            
            // level predictor info for pc 
            if(tmpAllLevelLP.find(pc)==tmpAllLevelLP.end()){
                tmpAllLevelLP[pc]=LevelPredictor();
            }

            if(globalAllLevelLP.find(pc)==globalAllLevelLP.end()){
                globalAllLevelLP[pc]=LevelPredictor();
            }

            bool firstRatio=true; 
            double currRatio;
            UInt64 prevMisses;
            for(auto uord: mp[pc])
            {
                PCStat pcStat= uord.second;
                UInt64 total = pcStat.getHitCount() + pcStat.getMissCount();
                Helper::Message msg = Helper::Message(-1,-1, static_cast<MemComponent::component_t>(uord.first), total, pcStat.getMissCount());
                
                if(firstRatio){
                    currRatio=(double)msg.gettotalMiss()/(double)msg.gettotalAccess();
                    firstRatio=false;
                }
                else{
                    currRatio=(double)msg.gettotalMiss()/(double)prevMisses;
                } 

                prevMisses=msg.gettotalMiss();
                if( total > 100 ){
                    if(currRatio > 0.5){// reason is that it should be significant to skip
                        tmpAllLevelLP[pc].addSkipLevel(msg.getLevel());
                        globalAllLevelLP[pc].addSkipLevel(msg.getLevel());
                        msg.addLevelSkip();
                    }
                }

                allMsg.push_back(msg);// can be used for logging
            }
            return allMsg;
        }
    };

}
#endif