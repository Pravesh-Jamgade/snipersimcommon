#ifndef N_BIT_BRANCH_PREDICTOR_H
#define N_BIT_BRANCH_PREDICTOR_H

#include "branch_predictor.h"

#include <vector>

class NBitBranchPredictor : public BranchPredictor
{
public:
   NBitBranchPredictor(String name, core_id_t core_id, UInt32 size, UInt32 history_length);//saurabh
   ~NBitBranchPredictor();

    bool predict(bool indirect, IntPtr ip, IntPtr target) //saurabh
    {
    
      UInt32 index = ip % current_state.size();
      bool temp = (current_state[index]>>(hl-1));
      //  _LOG_PRINT(Log::Warning,"hl %u current state %u %u %u %u", hl, index, current_state[index], state, temp);

      return temp;
    }

   void update(bool predicted, bool actual, bool indirect, IntPtr ip, IntPtr target)
   {
      updateCounters(predicted, actual);
      UInt32 index = ip % current_state.size();
      int max_state = (1 << hl) - 1;
      int middle_state = (1 << (hl-1));

   // for n-bit
   //     0: NT, 1: T
         if (actual == predicted)
         {
            if(current_state[index] == 0) 
                  current_state[index] +=0;                           //it will remain at 00.       000
            else if(current_state[index] == max_state) 
                  current_state[index] +=0;                           //it will remain at 2^n - 1.  2^3-1 = 111
            else if(current_state[index] < middle_state) 
                  current_state[index] = current_state[index] - 1;    //001 010 011   -1
            else if(current_state[index] >= middle_state)
                  current_state[index] = current_state[index] + 1;    //100 101 110   +1
         }
         else if(actual != predicted)
         {
            if(current_state[index] < middle_state)
                  current_state[index] = current_state[index] + 1;    //000 001 010 011 +1
            else if(current_state[index] >= middle_state)
                  current_state[index] = current_state[index] - 1;    //100 101 110 111 -1
         }
   }

private:
   
   std::vector<int> current_state;//saurabh
   UInt32 hl;//saurabh
};

#endif
