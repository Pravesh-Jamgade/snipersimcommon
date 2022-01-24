#ifndef __DRAM_CNTLR_INTERFACE_H
#define __DRAM_CNTLR_INTERFACE_H

#include "fixed_types.h"
#include "subsecond_time.h"
#include "hit_where.h"
#include "shmem_msg.h"

#include "boost/tuple/tuple.hpp"
#include "cache_helper.h"

class MemoryManagerBase;
class ShmemPerfModel;
class ShmemPerf;

class DramCntlrInterface
{
   protected:
      MemoryManagerBase* m_memory_manager;
      ShmemPerfModel* m_shmem_perf_model;
      UInt32 m_cache_block_size;

      UInt32 getCacheBlockSize() { return m_cache_block_size; }
      MemoryManagerBase* getMemoryManager() { return m_memory_manager; }
      ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }
   
   //[update]
   private:
   IntPtr eip;
   String name;
   String memLevelDebug;
   cache_helper::CacheHelper* cacheHelper;

   public:
      //[update]
      UInt64 totalAccess;
      UInt64 totalLoads;
      UInt64 totalStores;
      
      typedef enum
      {
         READ = 0,
         WRITE,
         NUM_ACCESS_TYPES
      } access_t;

      DramCntlrInterface(MemoryManagerBase* memory_manager, ShmemPerfModel* shmem_perf_model, UInt32 cache_block_size)
         : m_memory_manager(memory_manager)
         , m_shmem_perf_model(shmem_perf_model)
         , m_cache_block_size(cache_block_size)
      {}
      virtual ~DramCntlrInterface() {}

      virtual boost::tuple<SubsecondTime, HitWhere::where_t> getDataFromDram(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, ShmemPerf *perf, IntPtr eip) = 0;
      virtual boost::tuple<SubsecondTime, HitWhere::where_t> putDataToDram(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, IntPtr eip) = 0;

      void handleMsgFromTagDirectory(core_id_t sender, PrL1PrL2DramDirectoryMSI::ShmemMsg* shmem_msg);

      //[update]
      void setCacheHelper(cache_helper::CacheHelper* cacheHelper){this->cacheHelper = cacheHelper;}
      cache_helper::CacheHelper* getCacheHelper(){return cacheHelper;}

      void setEIP(IntPtr eip) {this->eip=eip;}
      IntPtr getEIP(){return this->eip;}

      void setName(String name){this->name=name;}
      String getName(){return this->name;}

      void setMemLevelDebug(String objectDebugName){this->memLevelDebug=objectDebugName;}
      String getMemLevelDebug(){return this->memLevelDebug;}

      void loggingDRAM(IntPtr addr, Core::mem_op_t mem_op);
};

#endif // __DRAM_CNTLR_INTERFACE_H
