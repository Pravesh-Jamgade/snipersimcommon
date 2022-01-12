#ifndef TPH
#define TPH

#include<vector>
#include<stdlib.h>

#include "pentium_m_branch_target_buffer.h"
#include "branch_predictor_return_value.h"

#include "simulator.h"
#include "branch_predictor.h"

#define IP_TO_INDEX(_ip) ((_ip >> 4) & 0x1ff)
class Helper
{
public:
    UInt32 get_index(IntPtr ip){
        return IP_TO_INDEX(ip);
    }
};

class Counter
{
    public:
    Counter(UInt32 state)
    {
        this->state=state;
    }
    ~Counter(){}
    
    UInt32 state;
};

class Table{
    public:
    
    Table(int size):upperLimit(size), history(size, Counter(size)){
        for(int i=0; i< size; i++){
            history[i].state=i%size;
        }
    }

    Table(int size, int count):upperLimit(count), history(size, Counter(count)){
        for(int i=0; i< size; i++){
            history[i].state=i%count;
        }
    }
    ~Table(){
        
    }

    UInt32 get_val(UInt32 index){
        return history[index].state;
    }

    bool get_status(UInt32 index){
        return history[index].state >= (UInt32)(upperLimit/2);
    }

    void shift_right(UInt32 index)
    {   
        UInt32 tmp = history[index].state>>1;;
        history[index].state=tmp;
    }

    void shift_left(UInt32 index)
    {  
        UInt32 tmp =history[index].state<<1; 
        if(tmp > upperLimit) return;
        history[index].state=tmp;
    }

    void update(UInt32 value,UInt32 index){
        history[index].state=value;
    }

    private:
    std::vector<Counter> history;
    int upperLimit;
    int lowerLimit=0;
};

class Global:public Table{
   
    public:
    Global(int size):Table(size){}
    Global(int size, int counter):Table(size, counter){}
    ~Global(){}
};

class Local:public Table{

    public:
    Local(int size):Table(size){}
    Local(int size, int counter):Table(size, counter){}
    ~Local(){}    
};

class TPredictorHelper:public BranchPredictor, Helper
{
    public:
    TPredictorHelper(String name, core_id_t core_id):BranchPredictor(name, core_id),
    lht(512), cpt(512, 4), gpt(512, 4), lpt(512, 4)
        , lpt_s(false), gpt_s(false), cpt_s(false){}

    ~TPredictorHelper(){}
    bool predict(bool indirect, IntPtr ip, IntPtr target)
    {
        // lookup in btb
        

        // lookup lht
        pc_index=get_index(ip);
        
        // check value and use it to lookup in lpt, and get prediction counter value
        lht_value_as_index =lht.get_val(pc_index);
        printf("index=%d\tlpt_index=%d\n", pc_index, lht_value_as_index);
        // ght by using ghr value as index
        // UInt32 gpt_pcounter_value =gpt.get_val(ghr);
        // UInt32 cpt_pcounter_value =cpt.get_val(ghr);

        lpt_s=lpt.get_status(lht_value_as_index);
        gpt_s=gpt.get_status(ghr);
        cpt_s=cpt.get_status(ghr);

        if(lpt_s == gpt_s && gpt_s==true )
        {
            return true;
        }
        else if(lpt_s == gpt_s && gpt_s==false )
        {
            return false;
        }
        return cpt_s;
        
        // if true
        // update: lht move right; lpt, gpt, cpt=prev(gpt) increment, ghr move right append T or N from front 

    }

    void update(bool predicted, bool actual, bool indirect, IntPtr ip, IntPtr target)
    {
        updateCounters(predicted, actual);
        printf("ip=%d\t target=%d", ip, target);
        btb.update(predicted,actual,indirect,ip,target);
        
        if(predicted==actual)
        {
            lpt.shift_left(lht_value_as_index);
            cpt.update(gpt.get_val(ghr), ghr);
            gpt.shift_left(ghr);
            UInt32 k=0x100;
            ghr=ghr>>1 | k;
            lht_value_as_index = lht_value_as_index>>1 |k;
            lht.update(pc_index, lht_value_as_index);
        }
        else
        {
            cpt.update(lpt.get_val(lht_value_as_index), ghr);
            lpt.shift_right(lht_value_as_index);
            gpt.shift_right(ghr);
            ghr=ghr>>1;
            lht.shift_right(pc_index);
        }
    }

    private:
    // Needed:
    // BTB: Branch Table Buffer
    PentiumMBranchTargetBuffer btb;

    // LHT: Local History Table
    // LPT: Local Prediction Table
    // GHT: Global History Buffer
    // CPT: Choice Predicition Table
    // GHR: Global History Register
    Global cpt,gpt;
    Local lht,lpt;
    UInt32 ghr=511;
    bool lpt_s;
    bool gpt_s;
    bool cpt_s;
    UInt32 pc_index;
    UInt32 lht_value_as_index;

};
#endif