#include "PCPredictor.h"
#include "log.h"
using namespace PCPredictorSpace;

int LPHelper::lp_unlock=2;
std::unordered_map<IntPtr, LevelPredictor> LPHelper::tmpAllLevelLP;

// caculate LP table for next epoc
void PCStatHelper::updateLPTable()
{
      for(auto pc: tmpAllLevelPCStat){
         std::vector<Helper::Message> allMsg=processEpocEndComputation(pc.first, tmpAllLevelPCStat, counter);
      }
}

bool PCStatHelper::learnFromPrevEpoc(IntPtr pc, MemComponent::component_t level)
{   
    auto findPC = LPHelper::tmpAllLevelLP.find(pc);
    if(findPC!=LPHelper::tmpAllLevelLP.end()){
        auto findPrePC = perPCperLevelperEpocLPPerf.find(pc);
        if(findPrePC!=perPCperLevelperEpocLPPerf.end()){

        }
    }
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

void PCStatHelper::logInit(){
      
      
      LPHelper::clearLPTable();
      
      updateLPTable();

      logPerEpocTotalAccess();
      
      logPerPCTotalCountPerEpoc();

      logPerEpocTotalAccess();

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