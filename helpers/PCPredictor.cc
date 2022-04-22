#include "PCPredictor.h"
using namespace PCPredictorSpace;

int LPHelper::lp_unlock=2;
std::unordered_map<IntPtr, LevelPredictor> LPHelper::tmpAllLevelLP;

bool PCStatHelper::learnFromPrevEpoc(IntPtr pc, MemComponent::component_t level)
{   
    auto findPC = LPHelper::tmpAllLevelLP.find(pc);
    if(findPC!=LPHelper::tmpAllLevelLP.end()){
        auto findPrePC = perPCperLevelperEpocLPPerf.find(pc);
        if(findPrePC!=perPCperLevelperEpocLPPerf.end()){
            
        }
    }
}

