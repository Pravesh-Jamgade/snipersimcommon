#ifndef BIMODAL_HELPER_H
#define BIMODAL_HELPER_H

#include "simple_bimodal_table.h"
#include "branch_predictor.h"

class BimodalPredictor:public BranchPredictor{
    public:
    BimodalPredictor(String name, core_id_t core_id):BranchPredictor(name, core_id){}
    ~BimodalPredictor(){};
    void update(bool predicted, bool actual, bool indirect, IntPtr ip, IntPtr target)
    {
        updateCounters(predicted, actual);
        m_bimodal_table.update(predicted, actual, indirect, ip, target);
    }

    bool predict(bool indirect, IntPtr ip, IntPtr target)
    {
        return m_bimodal_table.predict(indirect, ip, target);
    }

    private:
    PentiumMBimodalTable m_bimodal_table;
};
#endif
