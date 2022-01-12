#include "simulator.h"

#include "n_bit_branch_predictor.h"

NBitBranchPredictor::NBitBranchPredictor(String name, core_id_t core_id, UInt32 size, UInt32 history_length)
   : BranchPredictor(name, core_id)
   ,  current_state(size), hl(history_length)
{
//     _LOG_PRINT(Log::Warning,"history_len %u %u", hl, history_length);
}

NBitBranchPredictor::~NBitBranchPredictor()
{
}

// bool NBitBranchPredictor::predict(IntPtr ip, IntPtr target) //saurabh
// {
   
//    UInt32 index = ip % current_state.size();
//    UInt32 state = current_state[index];
//    bool temp = (current_state[index]>>(hl-1));
//   //  _LOG_PRINT(Log::Warning,"hl %u current state %u %u %u %u", hl, index, current_state[index], state, temp);

//    return temp;
// }

// void NBitBranchPredictor::update(bool predicted, bool actual, IntPtr ip, IntPtr target)
// {
//    updateCounters(predicted, actual);
//    UInt32 index = ip % current_state.size();
//     int max_state = (1 << hl) - 1;
//     int middle_state = (1 << (hl-1));

//    // for n-bit
//    //     0: NT, 1: T
//         if (actual == predicted)
//         {
//             if(current_state[index] == 0) 
//                 current_state[index] +=0;                           //it will remain at 00.       000
//             else if(current_state[index] == max_state) 
//                 current_state[index] +=0;                           //it will remain at 2^n - 1.  2^3-1 = 111
//             else if(current_state[index] < middle_state) 
//                 current_state[index] = current_state[index] - 1;    //001 010 011   -1
//             else if(current_state[index] >= middle_state)
//                 current_state[index] = current_state[index] + 1;    //100 101 110   +1
//         }
//         else if(actual != predicted)
//         {
//             if(current_state[index] < middle_state)
//                 current_state[index] = current_state[index] + 1;    //000 001 010 011 +1
//             else if(current_state[index] >= middle_state)
//                 current_state[index] = current_state[index] - 1;    //100 101 110 111 -1
//         }
        

    
// }
