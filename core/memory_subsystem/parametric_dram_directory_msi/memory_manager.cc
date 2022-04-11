#include "core_manager.h"
#include "memory_manager.h"
#include "cache_base.h"
#include "nuca_cache.h"
#include "dram_cache.h"
#include "tlb.h"
#include "simulator.h"
#include "log.h"
#include "dvfs_manager.h"
#include "itostr.h"
#include "instruction.h"
#include "config.hpp"
#include "distribution.h"
#include "topology_info.h"

#include <algorithm>

#include "config.h"
#include "mem_component.h"

#if 0
   extern Lock iolock;
#  include "core_manager.h"
#  include "simulator.h"
#  define MYLOG(...) { ScopedLock l(iolock); fflush(stderr); fprintf(stderr, "[%s] %d%cmm %-25s@%03u: ", itostr(getShmemPerfModel()->getElapsedTime()).c_str(), getCore()->getId(), Sim()->getCoreManager()->amiUserThread() ? '^' : '_', __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); }
#else
#  define MYLOG(...) {}
#endif


namespace ParametricDramDirectoryMSI
{

std::map<CoreComponentType, CacheCntlr*> MemoryManager::m_all_cache_cntlrs;

MemoryManager::MemoryManager(Core* core,
      Network* network, ShmemPerfModel* shmem_perf_model):
   MemoryManagerBase(core, network, shmem_perf_model),
   m_nuca_cache(NULL),
   m_dram_cache(NULL),
   m_dram_directory_cntlr(NULL),
   m_dram_cntlr(NULL),
   m_itlb(NULL), m_dtlb(NULL), m_stlb(NULL),
   m_tlb_miss_penalty(NULL,0),
   m_tlb_miss_parallel(false),
   m_tag_directory_present(false),
   m_dram_cntlr_present(false),
   m_enabled(false)
{

   cacheHelper = core->getCacheHelper();
   PCStatCollector = std::make_shared<Helper::PCStatHelper>();
   if(Sim()->getCfg()->hasKey("debug/epocNumber"))
   {
      debugEpoc=Sim()->getCfg()->getInt("debug/epocNumber");
      printf("DebugEpoc Counter=%ld\n", debugEpoc);
   }
   _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PC_STATUS, "pc,level,miss,total,skip\n");
  
   // Read Parameters from the Config file
   std::map<MemComponent::component_t, CacheParameters> cache_parameters;
   std::map<MemComponent::component_t, String> cache_names;

   bool nuca_enable = false;
   CacheParameters nuca_parameters;

   const ComponentPeriod *global_domain = Sim()->getDvfsManager()->getGlobalDomain();

   UInt32 smt_cores;
   bool dram_direct_access = false;
   UInt32 dram_directory_total_entries = 0;
   UInt32 dram_directory_associativity = 0;
   UInt32 dram_directory_max_num_sharers = 0;
   UInt32 dram_directory_max_hw_sharers = 0;
   String dram_directory_type_str;
   UInt32 dram_directory_home_lookup_param = 0;
   ComponentLatency dram_directory_cache_access_time(global_domain, 0);

   try
   {
      m_cache_block_size = Sim()->getCfg()->getInt("perf_model/l1_icache/cache_block_size");

      m_last_level_cache = (MemComponent::component_t)(Sim()->getCfg()->getInt("perf_model/cache/levels") - 2 + MemComponent::L2_CACHE);

      UInt32 stlb_size = Sim()->getCfg()->getInt("perf_model/stlb/size");
      if (stlb_size)
         m_stlb = new TLB("stlb", "perf_model/stlb", getCore()->getId(), stlb_size, Sim()->getCfg()->getInt("perf_model/stlb/associativity"), NULL);
      UInt32 itlb_size = Sim()->getCfg()->getInt("perf_model/itlb/size");
      if (itlb_size)
         m_itlb = new TLB("itlb", "perf_model/itlb", getCore()->getId(), itlb_size, Sim()->getCfg()->getInt("perf_model/itlb/associativity"), m_stlb);
      UInt32 dtlb_size = Sim()->getCfg()->getInt("perf_model/dtlb/size");
      if (dtlb_size)
         m_dtlb = new TLB("dtlb", "perf_model/dtlb", getCore()->getId(), dtlb_size, Sim()->getCfg()->getInt("perf_model/dtlb/associativity"), m_stlb);
      m_tlb_miss_penalty = ComponentLatency(core->getDvfsDomain(), Sim()->getCfg()->getInt("perf_model/tlb/penalty"));
      m_tlb_miss_parallel = Sim()->getCfg()->getBool("perf_model/tlb/penalty_parallel");

      smt_cores = Sim()->getCfg()->getInt("perf_model/core/logical_cpus");

      confName.clear();
      objName.clear();

      for(UInt32 i = MemComponent::FIRST_LEVEL_CACHE; i <= (UInt32)m_last_level_cache; ++i)
      {
         String configName, objectName;
         switch((MemComponent::component_t)i) {
            case MemComponent::L1_ICACHE:
               configName = "l1_icache";
               objectName = "L1-I";
               break;
            case MemComponent::L1_DCACHE:
               configName = "l1_dcache";
               objectName = "L1-D";
               break;
            default:
               String level = itostr(i - MemComponent::L2_CACHE + 2);
               configName = "l" + level + "_cache";
               objectName = "L" + level;
               break;
         }

         std::cout<<configName<<" "<<objectName<<std::endl;
         confName.push_back(configName);
         objName.push_back(objectName);
         
         const ComponentPeriod *clock_domain = NULL;
         String domain_name = Sim()->getCfg()->getStringArray("perf_model/" + configName + "/dvfs_domain", core->getId());
         if (domain_name == "core")
            clock_domain = core->getDvfsDomain();
         else if (domain_name == "global")
            clock_domain = global_domain;
         else
            LOG_PRINT_ERROR("dvfs_domain %s is invalid", domain_name.c_str());

         LOG_ASSERT_ERROR(Sim()->getCfg()->getInt("perf_model/" + configName + "/cache_block_size") == m_cache_block_size,
                          "The cache block size of the %s is not the same as the l1_icache (%d)", configName.c_str(), m_cache_block_size);

         cache_parameters[(MemComponent::component_t)i] = CacheParameters(
            configName,
            Sim()->getCfg()->getIntArray(   "perf_model/" + configName + "/cache_size", core->getId()),
            Sim()->getCfg()->getIntArray(   "perf_model/" + configName + "/associativity", core->getId()),
            getCacheBlockSize(),
            Sim()->getCfg()->getStringArray("perf_model/" + configName + "/address_hash", core->getId()),
            Sim()->getCfg()->getStringArray("perf_model/" + configName + "/replacement_policy", core->getId()),
            Sim()->getCfg()->getBoolArray(  "perf_model/" + configName + "/perfect", core->getId()),
            i == MemComponent::L1_ICACHE
               ? Sim()->getCfg()->getBoolArray(  "perf_model/" + configName + "/coherent", core->getId())
               : true,
            ComponentLatency(clock_domain, Sim()->getCfg()->getIntArray("perf_model/" + configName + "/data_access_time", core->getId())),
            ComponentLatency(clock_domain, Sim()->getCfg()->getIntArray("perf_model/" + configName + "/tags_access_time", core->getId())),
            ComponentLatency(clock_domain, Sim()->getCfg()->getIntArray("perf_model/" + configName + "/writeback_time", core->getId())),
            ComponentBandwidthPerCycle(clock_domain,
               i < (UInt32)m_last_level_cache
                  ? Sim()->getCfg()->getIntArray("perf_model/" + configName + "/next_level_read_bandwidth", core->getId())
                  : 0),
            Sim()->getCfg()->getStringArray("perf_model/" + configName + "/perf_model_type", core->getId()),
            Sim()->getCfg()->getBoolArray(  "perf_model/" + configName + "/writethrough", core->getId()),
            Sim()->getCfg()->getIntArray(   "perf_model/" + configName + "/shared_cores", core->getId()) * smt_cores,
            Sim()->getCfg()->getStringArray("perf_model/" + configName + "/prefetcher", core->getId()),
            i == MemComponent::L1_DCACHE
               ? Sim()->getCfg()->getIntArray(   "perf_model/" + configName + "/outstanding_misses", core->getId())
               : 0
         );
         cache_names[(MemComponent::component_t)i] = objectName;

         /* Non-application threads will be distributed at 1 per process, probably not as shared_cores per process.
            Still, they need caches (for inter-process data communication, not for modeling target timing).
            Make them non-shared so we don't create process-spanning shared caches. */
         if (getCore()->getId() >= (core_id_t) Sim()->getConfig()->getApplicationCores())
            cache_parameters[(MemComponent::component_t)i].shared_cores = 1;
      }

      nuca_enable = Sim()->getCfg()->getBoolArray(  "perf_model/nuca/enabled", core->getId());
      if (nuca_enable)
      {
         nuca_parameters = CacheParameters(
            "nuca",
            Sim()->getCfg()->getIntArray(   "perf_model/nuca/cache_size", core->getId()),
            Sim()->getCfg()->getIntArray(   "perf_model/nuca/associativity", core->getId()),
            getCacheBlockSize(),
            Sim()->getCfg()->getStringArray("perf_model/nuca/address_hash", core->getId()),
            Sim()->getCfg()->getStringArray("perf_model/nuca/replacement_policy", core->getId()),
            false, true,
            ComponentLatency(global_domain, Sim()->getCfg()->getIntArray("perf_model/nuca/data_access_time", core->getId())),
            ComponentLatency(global_domain, Sim()->getCfg()->getIntArray("perf_model/nuca/tags_access_time", core->getId())),
            ComponentLatency(global_domain, 0), ComponentBandwidthPerCycle(global_domain, 0), "", false, 0, "", 0 // unused
         );
      }

      // Dram Directory Cache
      dram_directory_total_entries = Sim()->getCfg()->getInt("perf_model/dram_directory/total_entries");
      dram_directory_associativity = Sim()->getCfg()->getInt("perf_model/dram_directory/associativity");
      dram_directory_max_num_sharers = Sim()->getConfig()->getTotalCores();
      dram_directory_max_hw_sharers = Sim()->getCfg()->getInt("perf_model/dram_directory/max_hw_sharers");
      dram_directory_type_str = Sim()->getCfg()->getString("perf_model/dram_directory/directory_type");
      dram_directory_home_lookup_param = Sim()->getCfg()->getInt("perf_model/dram_directory/home_lookup_param");
      dram_directory_cache_access_time = ComponentLatency(global_domain, Sim()->getCfg()->getInt("perf_model/dram_directory/directory_cache_access_time"));

      // Dram Cntlr
      dram_direct_access = Sim()->getCfg()->getBool("perf_model/dram/direct_access");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error reading memory system parameters from the config file");
   }

   m_user_thread_sem = new Semaphore(0);
   m_network_thread_sem = new Semaphore(0);

   std::vector<core_id_t> core_list_with_dram_controllers = getCoreListWithMemoryControllers();
   std::vector<core_id_t> core_list_with_tag_directories;
   String tag_directory_locations = Sim()->getCfg()->getString("perf_model/dram_directory/locations");

   if (tag_directory_locations == "dram")
   {
      // Place tag directories only at DRAM controllers
      core_list_with_tag_directories = core_list_with_dram_controllers;
   }
   else
   {
      SInt32 tag_directory_interleaving;

      // Place tag directores at each (master) cache
      if (tag_directory_locations == "llc")
      {
         tag_directory_interleaving = cache_parameters[m_last_level_cache].shared_cores;
      }
      else if (tag_directory_locations == "interleaved")
      {
         tag_directory_interleaving = Sim()->getCfg()->getInt("perf_model/dram_directory/interleaving") * smt_cores;
      }
      else
      {
         LOG_PRINT_ERROR("Invalid perf_model/dram_directory/locations value %s", tag_directory_locations.c_str());
      }

      for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); core_id += tag_directory_interleaving)
      {
         core_list_with_tag_directories.push_back(core_id);
      }
   }

   m_tag_directory_home_lookup = new AddressHomeLookup(dram_directory_home_lookup_param, core_list_with_tag_directories, getCacheBlockSize());
   m_dram_controller_home_lookup = new AddressHomeLookup(dram_directory_home_lookup_param, core_list_with_dram_controllers, getCacheBlockSize());

   // if (m_core->getId() == 0)
   //   printCoreListWithMemoryControllers(core_list_with_dram_controllers);

   if (find(core_list_with_dram_controllers.begin(), core_list_with_dram_controllers.end(), getCore()->getId()) != core_list_with_dram_controllers.end())
   {
      m_dram_cntlr_present = true;

      m_dram_cntlr = new PrL1PrL2DramDirectoryMSI::DramCntlr(this,
            getShmemPerfModel(),
            getCacheBlockSize());
      Sim()->getStatsManager()->logTopology("dram-cntlr", core->getId(), core->getId());

      if (Sim()->getCfg()->getBoolArray("perf_model/dram/cache/enabled", core->getId()))
      {
         m_dram_cache = new DramCache(this, getShmemPerfModel(), m_dram_controller_home_lookup, getCacheBlockSize(), m_dram_cntlr);
         Sim()->getStatsManager()->logTopology("dram-cache", core->getId(), core->getId());
      }
   }

   if (find(core_list_with_tag_directories.begin(), core_list_with_tag_directories.end(), getCore()->getId()) != core_list_with_tag_directories.end())
   {
      m_tag_directory_present = true;

      if (!dram_direct_access)
      {
         if (nuca_enable)
         {
            m_nuca_cache = new NucaCache(
               this,
               getShmemPerfModel(),
               m_tag_directory_home_lookup,
               getCacheBlockSize(),
               nuca_parameters);
            Sim()->getStatsManager()->logTopology("nuca-cache", core->getId(), core->getId());
         }

         m_dram_directory_cntlr = new PrL1PrL2DramDirectoryMSI::DramDirectoryCntlr(getCore()->getId(),
               this,
               m_dram_controller_home_lookup,
               m_nuca_cache,
               dram_directory_total_entries,
               dram_directory_associativity,
               getCacheBlockSize(),
               dram_directory_max_num_sharers,
               dram_directory_max_hw_sharers,
               dram_directory_type_str,
               dram_directory_cache_access_time,
               getShmemPerfModel());
         Sim()->getStatsManager()->logTopology("tag-dir", core->getId(), core->getId());
      }
   }

   /*There are 2 names for objects: config name and object name. We pass config name from cfg file. For that we find its object name.
   we then set that object name to all structures (call it target object name), we also set real object name of all structures (real object name).
   So when i want to log all l1d acccess, at l1 i will simply check if targetobjectname == realobjectname if yes then log. if targetobjectname=="" as in cfg 
   we pass empty configuration. Then we log all access to all objects in these case targetobjectname == "". should have simplified it! */   
   // normalizing either controller or cache of dram as dram access only
   String objectNameDebug,configNameDebug;
   // get objectName from configName, set objectName to each mem level. will be use to enbale debug/log
   String dramCacheConfigName = "dram-cache";
   String dramCntrlConfigName = "dram-cntrl";

   if(Sim()->getCfg()->hasKey("debug/DebugCacheLevel"))
   {
      configNameDebug = Sim()->getCfg()->getString("debug/DebugCacheLevel");
      
      if(configNameDebug == dramCacheConfigName){
         objectNameDebug = dramCacheConfigName;
      }
      else if(configNameDebug == dramCntrlConfigName){
         objectNameDebug = dramCntrlConfigName;
      }
      else{
         for(int i=0; i< confName.size(); i++)
         {
            if(configNameDebug == confName[i])
               objectNameDebug=objName[i];
         }
      }
   }
   std::cout<<"cache debug = "<<objectNameDebug<<" & "<<configNameDebug<<std::endl;

   for(UInt32 i = MemComponent::FIRST_LEVEL_CACHE; i <= (UInt32)m_last_level_cache; ++i) {
      CacheCntlr* cache_cntlr = new CacheCntlr(
         (MemComponent::component_t)i,
         cache_names[(MemComponent::component_t)i],
         getCore()->getId(),
         this,
         m_tag_directory_home_lookup,
         m_user_thread_sem,
         m_network_thread_sem,
         getCacheBlockSize(),
         cache_parameters[(MemComponent::component_t)i],
         getShmemPerfModel(),
         i == (UInt32)m_last_level_cache
      );
      
      //[update]
      cache_cntlr->setCacheHelper(cacheHelper);
      cache_cntlr->setEIP(-1);
      cache_cntlr->setName(cache_names[(MemComponent::component_t)i]);
      cache_cntlr->setMemLevelDebug(objectNameDebug);  // set objectName at all cacheCntrl to compare it with name of cache

      m_cache_cntlrs[(MemComponent::component_t)i] = cache_cntlr;
      setCacheCntlrAt(getCore()->getId(), (MemComponent::component_t)i, cache_cntlr);
   }

   //[update]
   // purpose of setMemleveldebug() name is to enable debug of that level only unless specified empty("")
   // each structre has object name; for given debug name we get its object name. for cache only i have object name. others i keep their config ass their object name
   // although unnecessary
   // by setting objectNameDebug for each strcuture, while logging i check object's own name and objectNameDebug if same, then log those accesses for that structure
   // for tlb and dram-cache, dram-cntr; we placing configname same as their individual objectname
   //objectNameDebug could be empty in case for debugging all levels
   if(m_dram_cntlr){
      m_dram_cntlr->setName(dramCntrlConfigName);// structures own name
      m_dram_cntlr->setMemLevelDebug(objectNameDebug);// debug levelname target name
      m_dram_cntlr->setCacheHelper(cacheHelper);
   }
      
   if(m_dram_cache){
      m_dram_cache->setName(dramCacheConfigName);// structures own name
      m_dram_cache->setMemLevelDebug(objectNameDebug);// debug levelname target name
      m_dram_cache->setCacheHelper(cacheHelper);
   }

   if(m_stlb)
   {
      m_stlb->setMemLevelDebug(objectNameDebug);// debug levelname target name
      m_stlb->setCacheHelper(cacheHelper);
   }
   if(m_dtlb)
   {
      m_dtlb->setMemLevelDebug(objectNameDebug);
      m_dtlb->setCacheHelper(cacheHelper);
   }
   if(m_itlb)
   {
      m_itlb->setMemLevelDebug(objectNameDebug);
      m_itlb->setCacheHelper(cacheHelper);
   }

   m_cache_cntlrs[MemComponent::L1_ICACHE]->setNextCacheCntlr(m_cache_cntlrs[MemComponent::L2_CACHE]);
   m_cache_cntlrs[MemComponent::L1_DCACHE]->setNextCacheCntlr(m_cache_cntlrs[MemComponent::L2_CACHE]);
   for(UInt32 i = MemComponent::L2_CACHE; i <= (UInt32)m_last_level_cache - 1; ++i)
      m_cache_cntlrs[(MemComponent::component_t)i]->setNextCacheCntlr(m_cache_cntlrs[(MemComponent::component_t)(i + 1)]);

   CacheCntlrList prev_cache_cntlrs;
   prev_cache_cntlrs.push_back(m_cache_cntlrs[MemComponent::L1_ICACHE]);
   prev_cache_cntlrs.push_back(m_cache_cntlrs[MemComponent::L1_DCACHE]);
   m_cache_cntlrs[MemComponent::L2_CACHE]->setPrevCacheCntlrs(prev_cache_cntlrs);

   for(UInt32 i = MemComponent::L2_CACHE; i <= (UInt32)m_last_level_cache - 1; ++i) {
      CacheCntlrList prev_cache_cntlrs;
      prev_cache_cntlrs.push_back(m_cache_cntlrs[(MemComponent::component_t)i]);
      m_cache_cntlrs[(MemComponent::component_t)(i + 1)]->setPrevCacheCntlrs(prev_cache_cntlrs);
   }

   // Create Performance Models
   for(UInt32 i = MemComponent::FIRST_LEVEL_CACHE; i <= (UInt32)m_last_level_cache; ++i)
      m_cache_perf_models[(MemComponent::component_t)i] = CachePerfModel::create(
       cache_parameters[(MemComponent::component_t)i].perf_model_type,
       cache_parameters[(MemComponent::component_t)i].data_access_time,
       cache_parameters[(MemComponent::component_t)i].tags_access_time
      );


   if (m_dram_cntlr_present)
      LOG_ASSERT_ERROR(m_cache_cntlrs[m_last_level_cache]->isMasterCache() == true,
                       "DRAM controllers may only be at 'master' node of shared caches\n"
                       "\n"
                       "Make sure perf_model/dram/controllers_interleaving is a multiple of perf_model/l%d_cache/shared_cores\n",
                       Sim()->getCfg()->getInt("perf_model/cache/levels")
                      );
   if (m_tag_directory_present)
      LOG_ASSERT_ERROR(m_cache_cntlrs[m_last_level_cache]->isMasterCache() == true,
                       "Tag directories may only be at 'master' node of shared caches\n"
                       "\n"
                       "Make sure perf_model/dram_directory/interleaving is a multiple of perf_model/l%d_cache/shared_cores\n",
                       Sim()->getCfg()->getInt("perf_model/cache/levels")
                      );


   // The core id to use when sending messages to the directory (master node of the last-level cache)
   m_core_id_master = getCore()->getId() - getCore()->getId() % cache_parameters[m_last_level_cache].shared_cores;

   if (m_core_id_master == getCore()->getId())
   {
      UInt32 num_sets = cache_parameters[MemComponent::L1_DCACHE].num_sets;
      // With heterogeneous caches, or fancy hash functions, we can no longer be certain that operations
      // only have effect within a set as we see it. Turn of optimization...
      if (num_sets != (1UL << floorLog2(num_sets)))
         num_sets = 1;
      for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
      {
         if (Sim()->getCfg()->getIntArray("perf_model/l1_dcache/cache_size", core_id) != cache_parameters[MemComponent::L1_DCACHE].size)
            num_sets = 1;
         if (Sim()->getCfg()->getStringArray("perf_model/l1_dcache/address_hash", core_id) != "mask")
            num_sets = 1;
         // FIXME: We really should check all cache levels
      }

      m_cache_cntlrs[(UInt32)m_last_level_cache]->createSetLocks(
         getCacheBlockSize(),
         num_sets,
         m_core_id_master,
         cache_parameters[m_last_level_cache].shared_cores
      );
      if (dram_direct_access && getCore()->getId() < (core_id_t)Sim()->getConfig()->getApplicationCores())
      {
         LOG_ASSERT_ERROR(Sim()->getConfig()->getApplicationCores() <= cache_parameters[m_last_level_cache].shared_cores, "DRAM direct access is only possible when there is just a single last-level cache (LLC level %d shared by %d, num cores %d)", m_last_level_cache, cache_parameters[m_last_level_cache].shared_cores, Sim()->getConfig()->getApplicationCores());
         LOG_ASSERT_ERROR(m_dram_cntlr != NULL, "I'm supposed to have direct access to a DRAM controller, but there isn't one at this node");
         m_cache_cntlrs[(UInt32)m_last_level_cache]->setDRAMDirectAccess(
            m_dram_cache ? (DramCntlrInterface*)m_dram_cache : (DramCntlrInterface*)m_dram_cntlr,
            Sim()->getCfg()->getInt("perf_model/llc/evict_buffers"));
      }
   }

   // Register Call-backs
   getNetwork()->registerCallback(SHARED_MEM_1, MemoryManagerNetworkCallback, this);

   // Set up core topology information
   getCore()->getTopologyInfo()->setup(smt_cores, cache_parameters[m_last_level_cache].shared_cores);
}

MemoryManager::~MemoryManager()
{

   UInt32 i;

   getNetwork()->unregisterCallback(SHARED_MEM_1);

   // Delete the Models

   if (m_itlb) delete m_itlb;
   if (m_dtlb) delete m_dtlb;
   if (m_stlb) delete m_stlb;

   for(i = MemComponent::FIRST_LEVEL_CACHE; i <= (UInt32)m_last_level_cache; ++i)
   {
      delete m_cache_perf_models[(MemComponent::component_t)i];
      m_cache_perf_models[(MemComponent::component_t)i] = NULL;
   }

   delete m_user_thread_sem;
   delete m_network_thread_sem;
   delete m_tag_directory_home_lookup;
   delete m_dram_controller_home_lookup;

   for(i = MemComponent::FIRST_LEVEL_CACHE; i <= (UInt32)m_last_level_cache; ++i)
   {
      delete m_cache_cntlrs[(MemComponent::component_t)i];
      m_cache_cntlrs[(MemComponent::component_t)i] = NULL;
   }

   if (m_nuca_cache)
      delete m_nuca_cache;
   if (m_dram_cache)
      delete m_dram_cache;
   if (m_dram_cntlr)
      delete m_dram_cntlr;
   if (m_dram_directory_cntlr)
      delete m_dram_directory_cntlr;
   
}

HitWhere::where_t
MemoryManager::coreInitiateMemoryAccess(
      MemComponent::component_t mem_component,
      Core::lock_signal_t lock_signal,
      Core::mem_op_t mem_op_type,
      IntPtr address, UInt32 offset,
      Byte* data_buf, UInt32 data_length,
      Core::MemModeled modeled,
      IntPtr eip)
{
   LOG_ASSERT_ERROR(mem_component <= m_last_level_cache,
      "Error: invalid mem_component (%d) for coreInitiateMemoryAccess", mem_component);

   String path;

   for(UInt32 i = MemComponent::FIRST_LEVEL_CACHE; i <= (UInt32)m_last_level_cache; ++i) {
      m_cache_cntlrs[(MemComponent::component_t)i]->setEIP(eip);
   }  

   if(m_dram_cache!=NULL){
      m_dram_cache->setEIP(eip);
   }
      
   if(m_dram_cntlr!=NULL){
      m_dram_cntlr->setEIP(eip);
   }

   if (mem_component == MemComponent::L1_ICACHE && m_itlb)
      accessTLB(m_itlb, address, true, modeled, eip, path);
   else if (mem_component == MemComponent::L1_DCACHE && m_dtlb)
      accessTLB(m_dtlb, address, false, modeled, eip, path);

   HitWhere::where_t hitWhere= m_cache_cntlrs[mem_component]->processMemOpFromCore(
         lock_signal,
         mem_op_type,
         address, offset,
         data_buf, data_length,
         modeled == Core::MEM_MODELED_NONE || modeled == Core::MEM_MODELED_COUNT ? false : true,
         modeled == Core::MEM_MODELED_NONE ? false : true,  eip, path, PCStatCollector);

   if(Cache::sendMsgFlag){
      for(auto pc: PCStatCollector->tmpAllLevelPCStat){
         std::vector<Helper::Message> allMsg = PCStatCollector->processEpocEndComputation(pc.first, PCStatCollector->tmpAllLevelPCStat);

         // if only debugEpoc is reached//done because right after first epoc could not seee effetive skipable levels
         if(debugEpoc==epocCounter.getCount()){

            // logging to find pc with their skippable levels
            for(auto msg: allMsg){
               _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_PC_STATUS, "%ld,%s,%ld,%ld,%ld\n", 
                  pc.first, 
                  MemComponent2String(msg.getLevel()).c_str(),
                  msg.gettotalMiss(),
                  msg.gettotalAccess(),
                  msg.isLevelSkipable()
               );
            } 
         }
      }

      if(debugEpoc==epocCounter.getCount())
      {
         printf("[enable] epocCounter=%ld, debugEpoc=%ld\n", epocCounter.getCount(), debugEpoc);
         printf("[lockreset] 2->1\n");
         // allow predictor measurement//allow accesses to look for LP
         PCStatCollector->lockreset();//2->1
      }

      // if(debugEpoc==epocCounter.getCount()-1){
         printf("[LP hitrate] epocNumber=%ld, hitrate=%f\n", epocCounter.getCount(),PCStatCollector->getLPHitRate());
         _LOG_CUSTOM_LOGGER(Log::Warning, Log::LogDst::LP_MISS_RATE, "Hit Rate=%f", PCStatCollector->getLPHitRate());
         // PCStatCollector->lockreset();//1->0// disable lookup to LP//experimenting for only one immediate epoc miss-rate check after key epoc where skip information is calculated
      // }

      PCStatCollector->reset();
      Cache::resetSendMsgFlag();
      epocCounter.increase();
   }

   return hitWhere;
}

void
MemoryManager::handleMsgFromNetwork(NetPacket& packet)
{
MYLOG("begin");
   core_id_t sender = packet.sender;
   PrL1PrL2DramDirectoryMSI::ShmemMsg* shmem_msg = PrL1PrL2DramDirectoryMSI::ShmemMsg::getShmemMsg((Byte*) packet.data, &m_dummy_shmem_perf);
   SubsecondTime msg_time = packet.time;

   getShmemPerfModel()->setElapsedTime(ShmemPerfModel::_SIM_THREAD, msg_time);
   shmem_msg->getPerf()->updatePacket(packet);

   MemComponent::component_t receiver_mem_component = shmem_msg->getReceiverMemComponent();
   MemComponent::component_t sender_mem_component = shmem_msg->getSenderMemComponent();

   if (m_enabled)
   {
      LOG_PRINT("Got Shmem Msg: type(%i), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), sender(%i), receiver(%i)",
            shmem_msg->getMsgType(), shmem_msg->getAddress(), sender_mem_component, receiver_mem_component, sender, packet.receiver);
   }

   switch (receiver_mem_component)
   {
      case MemComponent::L2_CACHE: /* PrL1PrL2DramDirectoryMSI::DramCntlr sends to L2 and doesn't know about our other levels */
      case MemComponent::LAST_LEVEL_CACHE:
         switch(sender_mem_component)
         {
            case MemComponent::TAG_DIR:
               m_cache_cntlrs[m_last_level_cache]->handleMsgFromDramDirectory(sender, shmem_msg);
               break;

            default:
               LOG_PRINT_ERROR("Unrecognized sender component(%u)",
                     sender_mem_component);
               break;
         }
         break;

      case MemComponent::TAG_DIR:
         LOG_ASSERT_ERROR(m_tag_directory_present, "Tag directory NOT present");

         switch(sender_mem_component)
         {
            case MemComponent::LAST_LEVEL_CACHE:
               m_dram_directory_cntlr->handleMsgFromL2Cache(sender, shmem_msg);
               break;

            case MemComponent::DRAM:
               m_dram_directory_cntlr->handleMsgFromDRAM(sender, shmem_msg);
               break;

            default:
               LOG_PRINT_ERROR("Unrecognized sender component(%u)",
                     sender_mem_component);
               break;
         }
         break;

      case MemComponent::DRAM:
         LOG_ASSERT_ERROR(m_dram_cntlr_present, "Dram Cntlr NOT present");

         switch(sender_mem_component)
         {
            case MemComponent::TAG_DIR:
            {
               DramCntlrInterface* dram_interface = m_dram_cache ? (DramCntlrInterface*)m_dram_cache : (DramCntlrInterface*)m_dram_cntlr;
               dram_interface->handleMsgFromTagDirectory(sender, shmem_msg);
               break;
            }

            default:
               LOG_PRINT_ERROR("Unrecognized sender component(%u)",
                     sender_mem_component);
               break;
         }
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized receiver component(%u)",
               receiver_mem_component);
         break;
   }

   // Delete the allocated Shared Memory Message
   // First delete 'data_buf' if it is present
   // LOG_PRINT("Finished handling Shmem Msg");

   if (shmem_msg->getDataLength() > 0)
   {
      assert(shmem_msg->getDataBuf());
      delete [] shmem_msg->getDataBuf();
   }
   delete shmem_msg;
MYLOG("end");
}

void
MemoryManager::sendMsg(PrL1PrL2DramDirectoryMSI::ShmemMsg::msg_t msg_type, MemComponent::component_t sender_mem_component, 
MemComponent::component_t receiver_mem_component, core_id_t requester, core_id_t receiver, IntPtr address, 
Byte* data_buf, UInt32 data_length, HitWhere::where_t where, ShmemPerf *perf, ShmemPerfModel::Thread_t thread_num)
{
MYLOG("send msg %u %ul%u > %ul%u", msg_type, requester, sender_mem_component, receiver, receiver_mem_component);
   assert((data_buf == NULL) == (data_length == 0));
   PrL1PrL2DramDirectoryMSI::ShmemMsg shmem_msg(msg_type, sender_mem_component, receiver_mem_component, 
   requester, address, data_buf, data_length, perf);
   shmem_msg.setWhere(where);

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   SubsecondTime msg_time = getShmemPerfModel()->getElapsedTime(thread_num);
   perf->updateTime(msg_time);

   if (m_enabled)
   {
      LOG_PRINT("Sending Msg: type(%u), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), requester(%i), sender(%i), receiver(%i)", msg_type, address, sender_mem_component, receiver_mem_component, requester, getCore()->getId(), receiver);
   }

   NetPacket packet(msg_time, SHARED_MEM_1,
         m_core_id_master, receiver,
         shmem_msg.getMsgLen(), (const void*) msg_buf);
   getNetwork()->netSend(packet);

   // Delete the Msg Buf
   delete [] msg_buf;
}

void
MemoryManager::broadcastMsg(PrL1PrL2DramDirectoryMSI::ShmemMsg::msg_t msg_type, MemComponent::component_t sender_mem_component, 
MemComponent::component_t receiver_mem_component, core_id_t requester, IntPtr address, Byte* data_buf, UInt32 data_length, 
ShmemPerf *perf, ShmemPerfModel::Thread_t thread_num)
{
MYLOG("bcast msg");
   assert((data_buf == NULL) == (data_length == 0));
   PrL1PrL2DramDirectoryMSI::ShmemMsg shmem_msg(msg_type, sender_mem_component, receiver_mem_component, 
   requester, address, data_buf, data_length, perf);

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   SubsecondTime msg_time = getShmemPerfModel()->getElapsedTime(thread_num);
   perf->updateTime(msg_time);

   if (m_enabled)
   {
      LOG_PRINT("Sending Msg: type(%u), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), requester(%i), sender(%i), receiver(%i)", msg_type, address, sender_mem_component, receiver_mem_component, requester, getCore()->getId(), NetPacket::BROADCAST);
   }

   NetPacket packet(msg_time, SHARED_MEM_1,
         m_core_id_master, NetPacket::BROADCAST,
         shmem_msg.getMsgLen(), (const void*) msg_buf);
   getNetwork()->netSend(packet);

   // Delete the Msg Buf
   delete [] msg_buf;
}

void
MemoryManager::accessTLB(TLB * tlb, IntPtr address, bool isIfetch, Core::MemModeled modeled, IntPtr eip, String& path)
{
   
   bool hit = tlb->lookup(address, getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_USER_THREAD));

   tlb->addRequest(eip,address,getCore()->getCycleCount(), getCore()->getId(), hit);

   cache_helper::Misc::pathAppend(path, tlb->name);
   cache_helper::Misc::stateAppend(hit, path);

   if (hit == false
       && !(modeled == Core::MEM_MODELED_NONE || modeled == Core::MEM_MODELED_COUNT)
       && m_tlb_miss_penalty.getLatency() != SubsecondTime::Zero()
   )
   {
      if (m_tlb_miss_parallel)
      {
         incrElapsedTime(m_tlb_miss_penalty.getLatency(), ShmemPerfModel::_USER_THREAD);
      }
      else
      {
         PseudoInstruction *i = new TLBMissInstruction(m_tlb_miss_penalty.getLatency(), isIfetch);
         getCore()->getPerformanceModel()->queuePseudoInstruction(i);
      }
   }
}

SubsecondTime
MemoryManager::getCost(MemComponent::component_t mem_component, CachePerfModel::CacheAccess_t access_type)
{
   if (mem_component == MemComponent::INVALID_MEM_COMPONENT)
      return SubsecondTime::Zero();

   return m_cache_perf_models[mem_component]->getLatency(access_type);
}

void
MemoryManager::incrElapsedTime(SubsecondTime latency, ShmemPerfModel::Thread_t thread_num)
{
   MYLOG("cycles += %s", itostr(latency).c_str());
   getShmemPerfModel()->incrElapsedTime(latency, thread_num);
}

void
MemoryManager::incrElapsedTime(MemComponent::component_t mem_component, CachePerfModel::CacheAccess_t access_type, ShmemPerfModel::Thread_t thread_num)
{
   incrElapsedTime(getCost(mem_component, access_type), thread_num);
}

void
MemoryManager::enableModels()
{
   m_enabled = true;

   for(UInt32 i = MemComponent::FIRST_LEVEL_CACHE; i <= (UInt32)m_last_level_cache; ++i)
   {
      m_cache_cntlrs[(MemComponent::component_t)i]->enable();
      m_cache_perf_models[(MemComponent::component_t)i]->enable();
   }

   if (m_dram_cntlr_present)
      m_dram_cntlr->getDramPerfModel()->enable();
}

void
MemoryManager::disableModels()
{
   m_enabled = false;

   for(UInt32 i = MemComponent::FIRST_LEVEL_CACHE; i <= (UInt32)m_last_level_cache; ++i)
   {
      m_cache_cntlrs[(MemComponent::component_t)i]->disable();
      m_cache_perf_models[(MemComponent::component_t)i]->disable();
   }

   if (m_dram_cntlr_present)
      m_dram_cntlr->getDramPerfModel()->disable();
}

}
