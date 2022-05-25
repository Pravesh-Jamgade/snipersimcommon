#ifndef CACHE_SET_RANDOM_H
#define CACHE_SET_RANDOM_H

#include "cache_set.h"

class CacheSetRandom : public CacheSet
{
   public:
      CacheSetRandom(CacheBase::cache_t cache_type,
            UInt32 associativity, UInt32 blocksize);
      ~CacheSetRandom();

      UInt32 getReplacementIndex(CacheCntlr *cntlr);
      UInt32 getReplacementIndex(CacheCntlr *cntlr, int& pos);
      void updateReplacementIndex(UInt32 accessed_index);
      virtual UInt32 getIndexLRUBits(UInt32 index);

   private:
      Random m_rand;
};

#endif /* CACHE_SET_RANDOM_H */
