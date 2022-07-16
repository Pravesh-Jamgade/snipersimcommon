#ifndef __MEM_COMPONENT_H__
#define __MEM_COMPONENT_H__

#include "fixed_types.h"

class MemComponent
{
   public:
      enum component_t
      {
         INVALID_MEM_COMPONENT = 0,
         MIN_MEM_COMPONENT,
         CORE = MIN_MEM_COMPONENT,
         FIRST_LEVEL_CACHE,
         L1_ICACHE = FIRST_LEVEL_CACHE,
         L1_DCACHE,
         L2_CACHE,
         L3_CACHE,
         L4_CACHE,
         /* more, unnamed stuff follows.
            make sure that MAX_MEM_COMPONENT < 32 as pr_l2_cache_block_info.h contains a 32-bit bitfield of these things
         */
         LAST_LEVEL_CACHE = 20,
         TAG_DIR,
         NUCA_CACHE,
         DRAM_CACHE,
         DRAM,
         MAX_MEM_COMPONENT = DRAM,
         NUM_MEM_COMPONENTS = MAX_MEM_COMPONENT - MIN_MEM_COMPONENT + 1
      };
      
      static String MemComponent2String(MemComponent::component_t mem_component)
      {
         switch(mem_component)
         {
            case MemComponent::CORE:         return "core";
            case MemComponent::L1_ICACHE:    return "l1i";
            case MemComponent::L1_DCACHE:    return "l1d";
            case MemComponent::L2_CACHE:     return "l2";
            case MemComponent::L3_CACHE:     return "l3";
            case MemComponent::L4_CACHE:     return "l4";
            case MemComponent::TAG_DIR:      return "directory";
            case MemComponent::NUCA_CACHE:   return "nuca-cache";
            case MemComponent::DRAM_CACHE:   return "dram-cache";
            case MemComponent::DRAM:         return "dram";
            default:                         return "????";
         }
      }
};
const char * MemComponentString(MemComponent::component_t mem_component);
#endif /* __MEM_COMPONENT_H__ */
