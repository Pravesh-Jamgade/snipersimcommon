#include "simulator.h"
#include "config.hpp"
#include "prefetcher.h"

#define NUM_IP_TABLE_L1_ENTRIES 1024                        // IP table entries 
#define NUM_GHB_ENTRIES 16                                  // Entries in the GHB
#define NUM_IP_INDEX_BITS 10                                // Bits to index into the IP table 
#define NUM_IP_TAG_BITS 6                                   // Tag bits per IP table entry
#define S_TYPE 1                                            // stream
#define CS_TYPE 2                                           // constant stride
#define CPLX_TYPE 3                                         // complex stride
#define NL_TYPE 4                                           // next line

// #define SIG_DEBUG_PRINT
#ifdef SIG_DEBUG_PRINT
#define SIG_DP(x) x
#else
#define SIG_DP(x)
#endif

class IP_TABLE_L1 {
  public:
    uint64_t ip_tag;
    uint64_t last_page;                                     // last page seen by IP
    uint64_t last_cl_offset;                                // last cl offset in the 4KB page
    int64_t last_stride;                                    // last delta observed
    uint16_t ip_valid;                                      // Valid IP or not   
    int conf;                                               // CS conf
    uint16_t signature;                                     // CPLX signature
    uint16_t str_dir;                                       // stream direction
    uint16_t str_valid;                                     // stream valid
    uint16_t str_strength;                                  // stream strength

    IP_TABLE_L1 () {
        ip_tag = 0;
        last_page = 0;
        last_cl_offset = 0;
        last_stride = 0;
        ip_valid = 0;
        signature = 0;
        conf = 0;
        str_dir = 0;
        str_valid = 0;
        str_strength = 0;
    };
};

class DELTA_PRED_TABLE {
public:
    int delta;
    int conf;

    DELTA_PRED_TABLE () {
        delta = 0;
        conf = 0;
    };        
};



class IPCP: public Prefetcher
{

    int number_of_cpus, prefetch_degree;
   
    IP_TABLE_L1 **trackers_l1;
    DELTA_PRED_TABLE **DPT_l1;
    uint64_t **ghb_l1;
    uint64_t *prev_cpu_cycle;
    uint64_t *num_misses;
    float *mpkc;
    int *spec_nl;


    IPCP(String configName, core_id_t core_id)
    : number_of_cpus(Sim()->getCfg()->getIntArray("perf_model/" + configName + "/prefetcher/ipcp/cores", core_id))
    , prefetch_degree(Sim()->getCfg()->getIntArray("perf_model/" + configName + "/prefetcher/ipcp/degree", core_id))
    {
      trackers_l1 = new IP_TABLE_L1*[number_of_cpus];
      DPT_l1 = new DELTA_PRED_TABLE*[number_of_cpus];
      ghb_l1 = new uint64_t*[number_of_cpus];
      
      for(int i=0; i< number_of_cpus; i++)
      {
          trackers_l1[i] = new IP_TABLE_L1[NUM_IP_TABLE_L1_ENTRIES];
          DPT_l1[i] = new DELTA_PRED_TABLE[4096];
          ghb_l1[i] = new uint64_t[NUM_GHB_ENTRIES];
      }

      prev_cpu_cycle = new uint64_t[number_of_cpus];
      num_misses = new uint64_t[number_of_cpus];
      mpkc = new float[number_of_cpus];
      spec_nl = new int[number_of_cpus];
    }

    /***************Updating the signature*************************************/ 
    uint16_t update_sig_l1(uint16_t old_sig, int delta){                           
        uint16_t new_sig = 0;
        int sig_delta = 0;

    // 7-bit sign magnitude form, since we need to track deltas from +63 to -63
        sig_delta = (delta < 0) ? (((-1) * delta) + (1 << 6)) : delta;
        new_sig = ((old_sig << 1) ^ sig_delta) & 0xFFF;                     // 12-bit signature

        return new_sig;
    }

    /****************Encoding the metadata***********************************/
    uint32_t encode_metadata(int stride, uint16_t type, int spec_nl){
    uint32_t metadata = 0;
    // first encode stride in the last 8 bits of the metadata
    if(stride > 0)
        metadata = stride;
    else
        metadata = ((-1*stride) | 0b1000000);
    // encode the type of IP in the next 4 bits
    metadata = metadata | (type << 8);
    // encode the speculative NL bit in the next 1 bit
    metadata = metadata | (spec_nl << 12);
    return metadata;
    }

    /*********************Checking for a global stream (GS class)***************/
    void check_for_stream_l1(int index, uint64_t cl_addr, uint8_t cpu){
        int pos_count=0, neg_count=0, count=0;
        uint64_t check_addr = cl_addr;

        // check for +ve stream
            for(int i=0; i<NUM_GHB_ENTRIES; i++){
                check_addr--;
                for(int j=0; j<NUM_GHB_ENTRIES; j++)
                    if(check_addr == ghb_l1[cpu][j]){
                        pos_count++;
                        break;
                    }
            }

        check_addr = cl_addr;
        // check for -ve stream
            for(int i=0; i<NUM_GHB_ENTRIES; i++){
                check_addr++;
                for(int j=0; j<NUM_GHB_ENTRIES; j++)
                    if(check_addr == ghb_l1[cpu][j]){
                        neg_count++;
                        break;
                    }
            }

            if(pos_count > neg_count){                                // stream direction is +ve
                trackers_l1[cpu][index].str_dir = 1;
                count = pos_count;
            }
            else{                                                     // stream direction is -ve
                trackers_l1[cpu][index].str_dir = 0;
                count = neg_count;
            }

        if(count > NUM_GHB_ENTRIES/2){                                // stream is detected
            trackers_l1[cpu][index].str_valid = 1;
            if(count >= (NUM_GHB_ENTRIES*3)/4)                        // stream is classified as strong if more than 3/4th entries belong to stream
                trackers_l1[cpu][index].str_strength = 1;
        }
        else{
            if(trackers_l1[cpu][index].str_strength == 0)             // if identified as weak stream, we need to reset
                trackers_l1[cpu][index].str_valid = 0;
        }

    }

    /**************************Updating confidence for the CS class****************/
    int update_conf(int stride, int pred_stride, int conf){
        if(stride == pred_stride){             // use 2-bit saturating counter for confidence
            conf++;
            if(conf > 3)
                conf = 3;
        } else {
            conf--;
            if(conf < 0)
                conf = 0;
        }

    return conf;
    }


};