#include "PCPredictor.h"
using namespace PCPredictorSpace;

int LPHelper::lp_unlock=2;
std::unordered_map<IntPtr, LevelPredictor> LPHelper::tmpAllLevelLP;
