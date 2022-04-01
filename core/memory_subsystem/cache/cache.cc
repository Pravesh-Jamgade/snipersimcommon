#include "simulator.h"
#include "config.hpp"
#include "cache.h"
#include "log.h"
#include "cache_helper.h"

// Cache class

//flag to recieve miss ratio
bool Cache::sendMsgFlag=false;

// constructors/destructors
Cache::Cache(
   String name,
   String cfgname,
   core_id_t core_id,
   UInt32 num_sets,
   UInt32 associativity,
   UInt32 cache_block_size,
   String replacement_policy,
   cache_t cache_type,
   hash_t hash,
   FaultInjector *fault_injector,
   AddressHomeLookup *ahl)
:  //[ORIGINAL] // cache_helper::CacheHelper(name, num_sets, associativity, cache_block_size, hash, ahl),
   CacheBase(name, num_sets, associativity, cache_block_size, hash, ahl),
   CacheAddonSpace::PCHistoryTable(Sim()->getCfg()->getInt("param/epoc")),
   m_enabled(false),
   m_num_accesses(0),
   m_num_hits(0),
   m_cache_type(cache_type),
   m_fault_injector(fault_injector)
{
   m_set_info = CacheSet::createCacheSetInfo(name, cfgname, core_id, replacement_policy, m_associativity);
   m_sets = new CacheSet*[m_num_sets];
   for (UInt32 i = 0; i < m_num_sets; i++)
   {
      m_sets[i] = CacheSet::createCacheSet(cfgname, core_id, replacement_policy, m_cache_type, m_associativity, m_blocksize, m_set_info);
   }

   // pcTable = new CacheAddonSpace::PCHistoryTable();
   #ifdef ENABLE_SET_USAGE_HIST
   m_set_usage_hist = new UInt64[m_num_sets];
   for (UInt32 i = 0; i < m_num_sets; i++)
      m_set_usage_hist[i] = 0;
   #endif
}

Cache::~Cache()
{
   #ifdef ENABLE_SET_USAGE_HIST
   printf("Cache %s set usage:", m_name.c_str());
   for (SInt32 i = 0; i < (SInt32) m_num_sets; i++)
      printf(" %" PRId64, m_set_usage_hist[i]);
   printf("\n");
   delete [] m_set_usage_hist;
   #endif

   if (m_set_info)
      delete m_set_info;

   for (SInt32 i = 0; i < (SInt32) m_num_sets; i++)
      delete m_sets[i];
   delete [] m_sets;
}

Lock&
Cache::getSetLock(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;

   splitAddress(addr, tag, set_index);
   assert(set_index < m_num_sets);

   return m_sets[set_index]->getLock();
}

bool
Cache::invalidateSingleLine(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;

   splitAddress(addr, tag, set_index);
   assert(set_index < m_num_sets);

   return m_sets[set_index]->invalidate(tag);
}

CacheBlockInfo*
Cache::accessSingleLine(IntPtr addr, access_t access_type,
      Byte* buff, UInt32 bytes, SubsecondTime now, bool update_replacement, String path, bool count)
{

   IntPtr tag;
   UInt32 set_index;
   UInt32 line_index = -1;
   UInt32 block_offset;

   splitAddress(addr, tag, set_index, block_offset);

   CacheSet* set = m_sets[set_index];
   CacheBlockInfo* cache_block_info = set->find(tag, &line_index);

   if (cache_block_info == NULL){
      return NULL;
   }

   //[update]
   // set cacheblockinfog to hotline
   processPCEntry(getEIP(),addr,cache_block_info,count);

   if (access_type == LOAD)
   {
      // NOTE: assumes error occurs in memory. If we want to model bus errors, insert the error into buff instead
      if (m_fault_injector)
         m_fault_injector->preRead(addr, set_index * m_associativity + line_index, bytes, (Byte*)m_sets[set_index]->getDataPtr(line_index, block_offset), now);

      set->read_line(line_index, block_offset, buff, bytes, update_replacement);
   }
   else
   {
      set->write_line(line_index, block_offset, buff, bytes, update_replacement);

      // NOTE: assumes error occurs in memory. If we want to model bus errors, insert the error into buff instead
      if (m_fault_injector)
         m_fault_injector->postWrite(addr, set_index * m_associativity + line_index, bytes, (Byte*)m_sets[set_index]->getDataPtr(line_index, block_offset), now);
   }

   return cache_block_info;
}

void
Cache::insertSingleLine(IntPtr addr, Byte* fill_buff,
      bool* eviction, IntPtr* evict_addr,
      CacheBlockInfo* evict_block_info, Byte* evict_buff,
      SubsecondTime now, CacheCntlr *cntlr, bool count)
{

   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   CacheBlockInfo* cache_block_info = CacheBlockInfo::create(m_cache_type);
   cache_block_info->setTag(tag);
   
   //[update]
   // set cacheblockinfog to hotline
   processPCEntry(getEIP(),addr,cache_block_info,count);
   
   m_sets[set_index]->insert(cache_block_info, fill_buff,
         eviction, evict_block_info, evict_buff, cntlr);
   *evict_addr = tagToAddress(evict_block_info->getTag());

   if (m_fault_injector) {
      // NOTE: no callback is generated for read of evicted data
      UInt32 line_index = -1;
      __attribute__((unused)) CacheBlockInfo* res = m_sets[set_index]->find(tag, &line_index);
      LOG_ASSERT_ERROR(res != NULL, "Inserted line no longer there?");

      m_fault_injector->postWrite(addr, set_index * m_associativity + line_index, m_sets[set_index]->getBlockSize(), (Byte*)m_sets[set_index]->getDataPtr(line_index, 0), now);
   }

   #ifdef ENABLE_SET_USAGE_HIST
   ++m_set_usage_hist[set_index];
   #endif

   delete cache_block_info;
}


// Single line cache access at addr
CacheBlockInfo*
Cache::peekSingleLine(IntPtr addr)
{
   IntPtr tag;
   UInt32 set_index;
   splitAddress(addr, tag, set_index);

   return m_sets[set_index]->find(tag);
}

void
Cache::updateCounters(bool cache_hit)
{
   if (m_enabled)
   {
      m_num_accesses ++;
      if (cache_hit)
         m_num_hits ++;
   }
}

void
Cache::updateHits(Core::mem_op_t mem_op_type, UInt64 hits)
{
   if (m_enabled)
   {
      m_num_accesses += hits;
      m_num_hits += hits;
   }
}

UInt64 Cache::getCycleCount(){return cycleNumber;}

bool Cache::processPCEntry(IntPtr pc, IntPtr addr, CacheBlockInfo* cache_block_info, bool count)
{
   IntPtr retAddr,retPC;
   retAddr=retPC=-1;
   if(getName() != "L1-D")
      return false;
   
   if(count){
      // printf("=[verify1] %ld, %ld, %s\n",getEIP(),addr,getName().c_str());
      cache_block_info->setOption(CacheBlockInfo::HOT_LINE);
      insert(pc,addr);
   }

   for(auto addr:action(getCycleCount()))//resetting table if counter == 0
   {
      IntPtr tag;
      UInt32 set_index;
      UInt32 line_index = -1;
      UInt32 block_offset;

      splitAddress(addr, tag, set_index, block_offset);

      CacheSet* set = m_sets[set_index];
      CacheBlockInfo* cache_block_info = set->find(tag, &line_index);
      if(cache_block_info!=NULL)
      {
         bool cool = cache_block_info->hasOption(CacheBlockInfo::HOT_LINE);
         if(cool)
         {
            cache_block_info->clearOption(CacheBlockInfo::HOT_LINE);
         }
      }
      if(!sendMsgFlag)
      {
         sendMsgFlag=true;
      }
   }
   return true;//if it is L1-D cache
}