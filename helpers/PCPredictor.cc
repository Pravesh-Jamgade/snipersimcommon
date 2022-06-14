#include "PCPredictor.h"
#include "log.h"
using namespace PCPredictorSpace;

int LPHelper::lp_unlock=2;
std::unordered_map<IntPtr, LevelPredictor> LPHelper::tmpAllLevelLP;

void PCStatHelper::LPLookup(IntPtr pc){
    if(LPHelper::isLockEnabled()!=1)
        return;
    // increase count if pc predicition found
    auto findPred = LPHelper::tmpAllLevelLP.find(pc);
    if(findPred!=LPHelper::tmpAllLevelLP.end()){
        lpEntryFound++;
    }
}

UInt64 PCStatHelper::getTotalLPAccessCount(){
    return lpEntryFound;
}

bool PCStatHelper::interpretMMratio(double ratio){
    return ratio>0.5111;
}

//at the end of epoc
void PCStatHelper::processEndPerformanceAnalysis(IntPtr pc, LevelPredictor levelP){

    // check if pc has missmatch from x epoc
    auto findPCx = perPCperLevelperEpocLPPerf.find(pc);

    if(findPCx!=perPCperLevelperEpocLPPerf.end()){
        
        // get per level missmatch ratio for pc
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

// caculate LP table for next epoc
void PCStatHelper::updateLPTable()
{
    // pick top ten pc from current epoc
    std::vector<std::pair<UInt64, IntPtr>> bag;
    
    for(auto pc: tmpAllLevelPCStat){
        bag.push_back(
            { perEpocperPCStat[pc.first].getCount(), pc.first}
        );
    }

    sort(bag.begin(), bag.end());
    
    std::vector<IntPtr> highpc;
    UInt64 thresold = getThreshold();
    for(auto it=bag.rbegin();it!=bag.rend();it++){
        if(it->first > thresold)
            highpc.push_back(it->second);
    }

    // performance analysis is LPT(x-1) vs MMT(x) decision deviation based type access count
    if(isLockEnabled()){
        for(auto pc: LPHelper::tmpAllLevelLP){
            processEndPerformanceAnalysis(pc.first, pc.second);
        }
    }

    // using high access pc for LPT update 
    for(auto high: highpc){
        processEpocEndComputation(high, tmpAllLevelPCStat, counter);
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
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_UNI_PC, "%d\n",core);
}

void PCStatHelper::logPerEpocSkiporNotSkipStatus(std::vector<Helper::Message>&allMsg, IntPtr pc){
    
}

void PCStatHelper::logTopVsTotalPC(){
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_TOP_VS_TOTAL_PC, "%ld,%d,%d,%d\n", counter, LPHelper::getTopPCcount(), getTotalPCCount(), core);
}

void PCStatHelper::logPerLevelLPVsTotalLPAccess(){
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_LP_VS_TOTAL_ACCESS, "%ld,%ld,%ld,%ld,%d \n", 
        counter,
        totalAccessPerEpoc,
        getTotalLPAccessCount(),
        howManyMatch.getCount(),
        core
    );
}

void PCStatHelper::logLPTopPCTotalAccessCount(){
    UInt64 sum=0;
    for(auto pc: LPHelper::tmpAllLevelLP){//top pc at end of current epoc for current epoc will be used in next epoc
        UInt64 total = 0;
        auto findPC = perEpocperPCStat.find(pc.first);
        if(findPC!=perEpocperPCStat.end())
            total=findPC->second.getCount();//before level l1
        sum += total;
    }
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_TOP_PC_ACCESS, "%ld,%ld,%d,%d\n", counter, sum, LPHelper::getTopPCcount(),core);
    
}

void PCStatHelper::logLPvsTypeAccess(){
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LP_VS_TYPE_ACCESS, "%ld,%ld,*,",counter,getTotalLPAccessCount());

    for(int level=MemComponent::L1_DCACHE; level<=llc; level++){
        MemComponent::component_t comp = static_cast<MemComponent::component_t>(level);
        auto findLevel = accessTypeCount.find(comp);
        if(findLevel!=accessTypeCount.end()){
         _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LP_VS_TYPE_ACCESS, "%ld,%ld,%ld,%ld,*,", 
            accessTypeCount[comp].getCount(LPPerf::fs),
            accessTypeCount[comp].getCount(LPPerf::ts),
            accessTypeCount[comp].getCount(LPPerf::fns),
            accessTypeCount[comp].getCount(LPPerf::tns)
          );
        }
        else{ _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LP_VS_TYPE_ACCESS, "0,0,0,0,*,");}
        
    }
    _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LP_VS_TYPE_ACCESS, "%d\n", core);

}

void PCStatHelper::logInit(){
      
    LPHelper::clearLPTable();
    if(LPHelper::getLockStatus() !=1){
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LP_VS_TYPE_ACCESS, "epoc,lp,p,fs1,ts1,fns1,tns1,q,fs2,ts2,fns2,tns2,r,fs3,ts3,fns3,tns3,s,core\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_TOP_PC_ACCESS, "epoc,toppcaccess,toppccount,core\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_LP_VS_TOTAL_ACCESS, "epoc,total,lp,accuracy,core\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_TOP_VS_TOTAL_PC, "epoc,top,total,core\n");
        _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::DEBUG_LEVEL_UNI_PC, "epoc,pc1,pc2,pc3,core\n");
       
        for(int i=MemComponent::component_t::L1_DCACHE; i<= llc; i++){
            
            MemComponent::component_t comp = static_cast<MemComponent::component_t>(i);
                perLevelLPperf[comp]=LPPerf();
        }
    } 

      updateLPTable();

      logLPvsTypeAccess();

      // top pc for currrent epoc x and their accesses
      logLPTopPCTotalAccessCount();
      // updated LP table will give top pc for current epoc
      logTopVsTotalPC();
      
      logPerLevelLPVsTotalLPAccess();

      logPerLevelPCCount();

      if(isLockEnabled()!=1){
        lockenable();
      }
      else{
      }
}