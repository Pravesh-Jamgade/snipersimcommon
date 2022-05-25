#ifndef CACHE_SET_NMRU_H
#define CACHE_SET_NMRU_H

#include "cache_set.h"

class CacheSetNMRU : public CacheSet
{
   public:
      CacheSetNMRU(CacheBase::cache_t cache_type,
            UInt32 associativity, UInt32 blocksize);
      ~CacheSetNMRU();

      UInt32 getReplacementIndex(CacheCntlr *cntlr);
      UInt32 getReplacementIndex(CacheCntlr *cntlr, int& pos);
      void updateReplacementIndex(UInt32 accessed_index);
      virtual UInt32 getIndexLRUBits(UInt32 index);

   private:
      UInt8* m_lru_bits;
      UInt8  m_replacement_pointer;
};

#endif /* CACHE_SET_NMRU_H */
