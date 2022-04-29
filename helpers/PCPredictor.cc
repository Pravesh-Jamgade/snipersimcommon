#include "PCPredictor.h"
#include "log.h"
using namespace PCPredictorSpace;

int LPHelper::lp_unlock=2;
std::unordered_map<IntPtr, LevelPredictor> LPHelper::tmpAllLevelLP;
std::unordered_map<IntPtr, LevelPredictor> LPHelper::copyTmpAllLevelLP;

bool PCStatHelper::interpretMMratio(double ratio){
    return ratio>0.5;
}

void PCStatHelper::processEndPerformanceAnalysis(IntPtr pc){
    // check if pc has decision from x-1 epoc
    auto findPC = LPHelper::copyTmpAllLevelLP.find(pc);
    if(findPC!=LPHelper::copyTmpAllLevelLP.end()){

        LevelPredictor levelP = findPC->second;

        // check if pc has missmatch from x epoc
        auto findPCx = perPCperLevelperEpocLPPerf.find(pc);

        if(findPCx!=perPCperLevelperEpocLPPerf.end()){
            
            // get per missmatch ratio for pc
            std::vector<EpocPerformanceStat> mmratio = findPCx->second;

            // iterate over each level and accumulate result levelwise
            for(int i =MemComponent::component_t::L1_DCACHE; i<= llc; i++){

                MemComponent::component_t comp = static_cast<MemComponent::component_t>(i);

                if(levelP.canSkipLevel(comp)){// skip
                    if(interpretMMratio(mmratio[i-MemComponent::component_t::L1_DCACHE].getMissRatio())){// mm high
                        perLevelLPperf[comp].inc(LPPerf::State::fs);//hazard
                    }
                    else{
                        perLevelLPperf[comp].inc(LPPerf::ts);//benefit
                    }
                }
                else{// no-skip
                    if(interpretMMratio(mmratio[i-MemComponent::component_t::L1_DCACHE].getMissRatio())){// mm high
                        perLevelLPperf[comp].inc(LPPerf::fns);//lost oppo
                    }else{
                        perLevelLPperf[comp].inc(LPPerf::tns);//benefit
                    }
                }  
            }
        }
        else return;
    }
     else return;
}

// caculate LP table for next epoc
void PCStatHelper::updateLPTable()
{
    for(auto pc: tmpAllLevelPCStat){
        if(isLockEnabled()){
            processEndPerformanceAnalysis(pc.first);
        }
        std::vector<Helper::Message> allMsg=processEpocEndComputation(pc.first, tmpAllLevelPCStat, counter);
        logPerEpocSkiporNotSkipStatus(allMsg, pc.first);
    }
}

bool PCStatHelper::learnFromPrevEpoc(IntPtr pc, MemComponent::component_t level)
{   
    auto findPC = perPCperLevelperEpocLPPerf.find(pc);
    if(findPC!=perPCperLevelperEpocLPPerf.end()){
        EpocPerformanceStat ep = findPC->second[level];        
        double missratio = ep.getMissRatio();
        if(missratio>0.5){
            return false;//dont skip
        }
    }
    return true;//skip
}

void PCStatHelper::logPerLevelPerEpocAccessThreshold()
{
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_THRESHOLD, "%ld,", counter);
    for(int i=MemComponent::component_t::L1_DCACHE;i<= llc; i++)
    {   
        auto findLvl = perLevelAccessCount.find(i);
        if(findLvl!=perLevelAccessCount.end())
        {
            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_THRESHOLD, "%ld,", getThresholdByLevel(i) );
        }
        else _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_THRESHOLD, "0,");
    }
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_THRESHOLD, "\n");
}

void PCStatHelper::logPerLevelPCCount()
{
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_UNI_PC, "%ld,", counter);
    for(int i=MemComponent::component_t::L1_DCACHE;i<= llc; i++)
    {   
        auto findLvl = perLevelUniqPC.find(i);
        if(findLvl!=perLevelUniqPC.end())
        {
            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_UNI_PC, "%ld,", findLvl->second.size());
        }
        else _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_UNI_PC, "0,");
    }
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_UNI_PC, "\n");
}

// LOGGERS

void PCStatHelper::logPerEpocTotalAccess(){
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_EPOC, "%ld,%ld,%ld\n",
        counter,getTotalAccessInCurrEpoc(),getThreshold());
}

void PCStatHelper::logPerPCTotalCountPerEpoc(){
      for(auto pc: perEpocperPCStat){
         _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_PC_PER_EPOC, "%ld,%ld,%ld\n", counter, pc.first, pc.second.getCount())
      }
}

void PCStatHelper::logPerLevelAccessCountPerEpoc(){
      _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_LEVEL_PER_EPOC, "%ld,", llc);
      for(int level=MemComponent::component_t::L1_DCACHE; level<= llc; level++){
         auto perLevelCount = perLevelAccessCount.find(level);
         if(perLevelCount!=perLevelAccessCount.end()){
            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_LEVEL_PER_EPOC, "%ld,", 
               perLevelCount->second.getCount());
         }
         else _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_LEVEL_PER_EPOC, "0,");
      }
      _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_LEVEL_PER_EPOC, "\n");

}

void PCStatHelper::logPerPCPerLevelAccessCountperEpoc(){
      for(auto pc : tmpAllLevelPCStat){
         _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_PC_PER_LEVEL_PER_EPOC, "%ld,%ld,", 
            counter,pc.first);
         for(int level=MemComponent::component_t::L1_DCACHE; level<= llc; level++){
            auto findLevel = pc.second.find(level);
            if(findLevel!=pc.second.end()){
               _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_PC_PER_LEVEL_PER_EPOC, "%ld,", 
                findLevel->second.getTotalCount());
            }
            else _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_PC_PER_LEVEL_PER_EPOC, "0,");
         }
         _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_PC_PER_LEVEL_PER_EPOC, "\n");
      }
}

void PCStatHelper::logLPTablePerEpoc(){
      for(auto pc: PCPredictorSpace::LPHelper::tmpAllLevelLP){
         auto findPC = tmpAllLevelPCStat.find(pc.first);
         if(findPC!= tmpAllLevelPCStat.end()){
            for(auto levelStat: findPC->second){
               _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_LOCAL_PC_STAT, 
                  "%ld,%ld,%s,%ld,%ld,%ld,%f\n", 

                  counter, 
                  pc.first,
                  MemComponent2String(static_cast<MemComponent::component_t>(levelStat.first)).c_str(), 
                  levelStat.second.getMissCount(),
                  levelStat.second.getHitCount(),
                  levelStat.second.getTotalCount(),
                  levelStat.second.getMissRatio()
               );
            }
         }
      }
}

void PCStatHelper::logPerPCPerLevelMissratioPerEpoc(){
    for(auto pc=perPCperLevelperEpocLPPerf.begin(); 
        pc!=perPCperLevelperEpocLPPerf.end(); pc++){
    
    if(perPCperLevelperEpocLPPerf[pc->first].size() == 0){
        continue;
    }
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_LOCAL_PER_PC_PER_MEM_LEVEL_PERF, "%ld,%ld,", 
        counter, pc->first);    
    
    for(auto levelPerf=perPCperLevelperEpocLPPerf[pc->first].begin(); 
        levelPerf!=perPCperLevelperEpocLPPerf[pc->first].end(); levelPerf++){
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_LOCAL_PER_PC_PER_MEM_LEVEL_PERF, "%f,", levelPerf->getMissRatio());
    }
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_LOCAL_PER_PC_PER_MEM_LEVEL_PERF, "\n");
    }
}

void PCStatHelper::logPerEpocSkiporNotSkipStatus(std::vector<Helper::Message>&allMsg, IntPtr pc){
    
}

void PCStatHelper::logPerEpocSkiporNotskipWhileEpoc()
{
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_NOSKIP_PER_EPOC, "%ld,", counter);
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_SKIP_PER_EPOC, "%ld,", counter);

    UInt64 total = 0;
    
    for(auto level: perLevelSkipWhileEpoc){
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_SKIP_PER_EPOC, "%d,", level.second.getCount());
        total += level.second.getCount();
    }

    // // ratio per level skip
    // for(auto level: perLevelSkipWhileEpoc){
    //     double ratio = (double)level.second.getCount()/(double)total;
    //     _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PER_LEVEL_SKIP_RATIO, "%f,", ratio);
    // }

    total = 0;
    for(auto level: perLevelNoSkipWhileEpoc){
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_NOSKIP_PER_EPOC, "%d,", level.second.getCount());
        total += level.second.getCount();
    }

    // // ratio per level noskip
    // for(auto level: perLevelNoSkipWhileEpoc){
    //     double ratio = (double)level.second.getCount()/(double)total;
    //     _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PER_LEVEL_NSKIP_RATIO, "%f,", ratio);
    // }
    
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_NOSKIP_PER_EPOC, "\n");
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_SKIP_PER_EPOC, "\n");
}

void PCStatHelper::logLPPerfPerLevel(){
    for(auto level: perLevelLPperf){
        if(level.first == MemComponent::L1_DCACHE){
            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PERF_L1, "%ld,%ld,%ld,%ld,%ld\n", 
                counter,
                level.second.getCount(LPPerf::fs),
                level.second.getCount(LPPerf::ts),
                level.second.getCount(LPPerf::fns),
                level.second.getCount(LPPerf::tns)
            );
        }

        if(level.first == MemComponent::L2_CACHE){
            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PERF_L2, "%ld,%ld,%ld,%ld,%ld\n", 
                counter,
                level.second.getCount(LPPerf::fs),
                level.second.getCount(LPPerf::ts),
                level.second.getCount(LPPerf::fns),
                level.second.getCount(LPPerf::tns)
            );
        }

        if(level.first == llc){
            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PERF_L3, "%ld,%ld,%ld,%ld,%ld\n", 
                counter,
                level.second.getCount(LPPerf::fs),
                level.second.getCount(LPPerf::ts),
                level.second.getCount(LPPerf::fns),
                level.second.getCount(LPPerf::tns)
            );
        }
        
    }
}

void PCStatHelper::logInit(){
      
    LPHelper::clearLPTable();
    if(LPHelper::getLockStatus() !=1){
        
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PERF_L1, "epoc,fs,ts,fns,tns\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PERF_L2, "epoc,fs,ts,fns,tns\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PERF_L3, "epoc,fs,ts,fns,tns\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_UNI_PC, "epoc,pc1,pc2,pc3,#unique pc count per level\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_PER_EPOC_N_SKIP, "epoc,skip,actual\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::MEM_GLOBAL_STATUS, "pc,level,miss,total,skip\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_LOCAL_PC_STAT, "epoc,pc,level,misses,hits,total,missratio\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_PC_PER_EPOC, "epoc,pc,totalAccess\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_EPOC, "epoc,totalAccess,threshold\n");

        // _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PER_LEVEL_SKIP_RATIO,"epoc,");
        // _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PER_LEVEL_NSKIP_RATIO,"epoc,");

        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_SKIP_PER_EPOC, "epoc,");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_NOSKIP_PER_EPOC, "epoc,");

        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_LOCAL_PER_PC_PER_MEM_LEVEL_PERF, "epoc,pc,");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_LEVEL_PER_EPOC, "epoc,");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_PC_PER_LEVEL_PER_EPOC, "epoc,pc,");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_THRESHOLD,"epoc,");
        for(int i=MemComponent::component_t::L1_DCACHE; i<= llc; i++){
            
            MemComponent::component_t comp = static_cast<MemComponent::component_t>(i);

            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_LOCAL_PER_PC_PER_MEM_LEVEL_PERF, "%s,", 
                MemComponent2String(comp).c_str());

            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_LEVEL_PER_EPOC, "%s,", 
                MemComponent2String(comp).c_str());

            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_PC_PER_LEVEL_PER_EPOC, "%s,", 
                MemComponent2String(comp).c_str());
            
            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_SKIP_PER_EPOC, "%s,", 
                MemComponent2String(comp).c_str());
            
            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_NOSKIP_PER_EPOC, "%s,", 
                MemComponent2String(comp).c_str());
            
            _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_THRESHOLD,"%s,",
                MemComponent2String(comp).c_str());
            
            // _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PER_LEVEL_SKIP_RATIO,"%s,",
            //     MemComponent2String(comp).c_str());

            // _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PER_LEVEL_NSKIP_RATIO,"%s,",
            //     MemComponent2String(comp).c_str());

                perLevelNoSkip[comp]=Helper::Counter(0);
                perLevelSkip[comp]=Helper::Counter(0);
                perLevelNoSkipWhileEpoc[comp]=Helper::Counter(0);
                perLevelSkipWhileEpoc[comp]=Helper::Counter(0);
                perLevelLPperf[comp]=LPPerf();
        }

        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_LOCAL_PER_PC_PER_MEM_LEVEL_PERF, "#per epoc per level missmatch ratio\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_LEVEL_PER_EPOC, "#total access per level per epoc\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_TOTAL_ACCESS_PER_PC_PER_LEVEL_PER_EPOC, "#total access per pc per level per epoc\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_SKIP_PER_EPOC, "#per level skip per epoc\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_NOSKIP_PER_EPOC, "#per level noskip per epoc\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_THRESHOLD,"#per level per epoc access threshold\n");

        // _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PER_LEVEL_SKIP_RATIO,"\n");

        // _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PER_LEVEL_NSKIP_RATIO,"\n");
    } 

      logPerEpocSkiporNotskipWhileEpoc();

      updateLPTable();

      logLPPerfPerLevel();

      logPerEpocTotalAccess();
      
      logPerPCTotalCountPerEpoc();

      logPerPCPerLevelAccessCountperEpoc();

      logPerLevelAccessCountPerEpoc();

      logPerLevelPerEpocAccessThreshold();

      logPerLevelPCCount();

      logLPTablePerEpoc();

      if(isLockEnabled()!=1){
        lockenable();
      }
      else{
        logPerPCPerLevelMissratioPerEpoc();
      }
}