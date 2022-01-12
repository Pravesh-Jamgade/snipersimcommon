#include "simulator.h"
#include "branch_predictor.h"
#include "one_bit_branch_predictor.h"
#include "n_bit_branch_predictor.h"  //saurabh
#include "pentium_m_branch_predictor.h"
#include "bimodal-helper.h" // pravesh
#include "TPredictorHelper.h" // pravesh
#include "config.hpp"
#include "stats.h"
#include "1bit_predictor.h"
#include "2bit_predictor.h"

BranchPredictor::BranchPredictor()
   : m_correct_predictions(0)
   , m_incorrect_predictions(0)
{
}

BranchPredictor::BranchPredictor(String name, core_id_t core_id)
   : m_correct_predictions(0)
   , m_incorrect_predictions(0)
{
  registerStatsMetric(name, core_id, "num-correct", &m_correct_predictions);
  registerStatsMetric(name, core_id, "num-incorrect", &m_incorrect_predictions);
}

BranchPredictor::~BranchPredictor()
{ }

UInt64 BranchPredictor::m_mispredict_penalty;

BranchPredictor* BranchPredictor::create(core_id_t core_id)
{
   try
   {
      config::Config *cfg = Sim()->getCfg();
      assert(cfg);

      m_mispredict_penalty = cfg->getIntArray("perf_model/branch_predictor/mispredict_penalty", core_id);

      String type = cfg->getStringArray("perf_model/branch_predictor/type", core_id);
      if (type == "none")
      {
         return 0;
      }
      else if (type == "one_bit")
      {
         UInt32 size = cfg->getIntArray("perf_model/branch_predictor/size", core_id);
         return new OneBitBranchPredictor("branch_predictor", core_id, size);
      }
      else if (type == "n_bit")   //saurabh
      {
         printf("[XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX] inside n-bit\n");
         UInt32 size = cfg->getIntArray("perf_model/branch_predictor/size", core_id); //saurabh
         UInt32 history_length = cfg->getIntArray("perf_model/branch_predictor/history_length", core_id); //saurabh
         return new NBitBranchPredictor("branch_predictor", core_id, size, history_length);  //saurabh
      }
      else if (type == "pentium_m")
      {
         BranchPredictor *ret = new PentiumMBranchPredictor("branch_predictor", core_id);
         printf("correct=%d\t incorrect=%d\t penalty=%d\n", ret->m_correct_predictions, ret->m_incorrect_predictions, BranchPredictor::m_mispredict_penalty);
         return ret;
      }
      else if (type == "bimodal")
      {
         return new BimodalPredictor("branch_predictor", core_id);//pravesh
      }
      else if (type == "tournament")
      {
         // printf("[ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ] inside tournament pred\n");
         BranchPredictor *ret = new TPredictorHelper("branch_predictor", core_id);//pravesh
         printf("correct=%d\t incorrect=%d\t penalty=%d\n", ret->m_correct_predictions, ret->m_incorrect_predictions, BranchPredictor::m_mispredict_penalty);
         return ret;
      }
      else if (type == "1bittest")
      {
         UInt32 size = cfg->getIntArray("perf_model/branch_predictor/size", core_id);
         return new OneBitPredictor("branch_predictor", core_id, size);//pravesh
      }
      else if (type == "2bittest")
      {
         UInt32 size = cfg->getIntArray("perf_model/branch_predictor/size", core_id);
         UInt32 counter_bits = cfg->getIntArray("perf_model/branch_predictor/bits", core_id);
         return new TwobitSpace::TwoBitPredictor("branch_predictor", core_id, size, counter_bits);//pravesh
      }
      else
      {
         LOG_PRINT_ERROR("Invalid branch predictor type.");
         return 0;
      }
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Config info not available while constructing branch predictor.");
      return 0;
   }
}

UInt64 BranchPredictor::getMispredictPenalty()
{
   return m_mispredict_penalty;
}

void BranchPredictor::resetCounters()
{
  m_correct_predictions = 0;
  m_incorrect_predictions = 0;
}

void BranchPredictor::updateCounters(bool predicted, bool actual)
{
   if (predicted == actual)
      ++m_correct_predictions;
   else
      ++m_incorrect_predictions;
}
