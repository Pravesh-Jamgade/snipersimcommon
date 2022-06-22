#ifndef CACHE_H
#define CACHE_H

#include "cache_base.h"
#include "cache_set.h"
#include "cache_block_info.h"
#include "utils.h"
#include "hash_map_set.h"
#include "cache_perf_model.h"
#include "shmem_perf_model.h"
#include "log.h"
#include "core.h"
#include "fault_injection.h"

//
#include "DeadBlockAnalysis.h"

// Define to enable the set usage histogram
//#define ENABLE_SET_USAGE_HIST

class Cache : public CacheBase
{
   private:
      bool m_enabled;

      // Cache counters
      UInt64 m_num_accesses;
      UInt64 m_num_hits;

      // Generic Cache Info
      cache_t m_cache_type;
      CacheSet** m_sets;
      CacheSetInfo* m_set_info;

      FaultInjector *m_fault_injector;

      core_id_t core_id;
      bool shared;

      //update
      UInt64 count_dead_blocks;
      bool loggedByOtherCore;
      double avg_dead_blocks_per_set;
      bool addFirstLine;

      #ifdef ENABLE_SET_USAGE_HIST
      UInt64* m_set_usage_hist;
      #endif

   public:

      // constructors/destructors
      Cache(
            bool isShared,
            String name,
            String cfgname,
            core_id_t core_id,
            UInt32 num_sets,
            UInt32 associativity, UInt32 cache_block_size,
            String replacement_policy,
            cache_t cache_type,
            hash_t hash = CacheBase::HASH_MASK,
            FaultInjector *fault_injector = NULL,
            AddressHomeLookup *ahl = NULL);
      ~Cache();

      Lock& getSetLock(IntPtr addr);

      bool invalidateSingleLine(IntPtr addr);
      CacheBlockInfo* accessSingleLine(IntPtr addr,
            access_t access_type, Byte* buff, UInt32 bytes, SubsecondTime now, bool update_replacement);
      void insertSingleLine(IntPtr addr, Byte* fill_buff,
            bool* eviction, IntPtr* evict_addr,
            CacheBlockInfo* evict_block_info, Byte* evict_buff, SubsecondTime now, CacheCntlr *cntlr = NULL);
      CacheBlockInfo* peekSingleLine(IntPtr addr);

      CacheBlockInfo* peekBlock(UInt32 set_index, UInt32 way) const { return m_sets[set_index]->peekBlock(way); }

      // Update Cache Counters
      void updateCounters(bool cache_hit);
      void updateHits(Core::mem_op_t mem_op_type, UInt64 hits);

      void enable() { m_enabled = true; }
      void disable() { m_enabled = false; }

      UInt64 getCycle();
      
      String logCache();
      bool allowed(){
            if(m_name == logCache())
                  return true;
            if(logCache() == "x"){
                  if(m_name == "L1-D" || m_name == "L2" || m_name =="L3")
                        return true;
            }
            return false;
      }

      bool isShared(){
            return shared;
      }

      void logAndClear(UInt64 epoc, UInt64 numShCores);

      // to avoid any duplicacy 
      bool isLoggedByOtherCore(){
           return loggedByOtherCore;
      }
};

template <class T>
UInt32 moduloHashFn(T key, UInt32 hash_fn_param, UInt32 num_buckets)
{
   return (key >> hash_fn_param) % num_buckets;
}

#endif /* CACHE_H */
