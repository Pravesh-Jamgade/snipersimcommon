#ifndef CACHE_SET_MRU_H
#define CACHE_SET_MRU_H

#include "cache_set.h"

class CacheSetMRU : public CacheSet
{
   public:
      CacheSetMRU(CacheBase::cache_t cache_type,
            UInt32 associativity, UInt32 blocksize);
      ~CacheSetMRU();

      UInt32 getReplacementIndex(CacheCntlr *cntlr);
      UInt32 getReplacementIndex(CacheCntlr *cntlr, int& pos);
      void updateReplacementIndex(UInt32 accessed_index);
      virtual UInt32 getIndexLRUBits(UInt32 index);

   private:
      UInt8* m_lru_bits;
};

#endif /* CACHE_SET_MRU_H */
