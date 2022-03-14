#include "cache_addon.h"
#include <algorithm>
#include <map>

using namespace CacheAddonSpace;

void PCHistoryTable::addEntry(IntPtr pc, IntPtr addr, IntPtr& returnPC, IntPtr& returnAddr){
    std::map<IntPtr, AddressHistory*>::iterator it;
    returnPC=returnAddr=-1;
    bool notfound=true;
    if(!valid())
    {
          // do table replacement
          // LRU
          int mN=1e9;
          // finding replacable pc based on lowest count of total addresses 
          for(auto e: table)
          {
              int count=e.second->getTotalAccessCount();
              mN=std::min(mN, count);
              if(count != mN)
              {
                  returnPC=e.first;
              }
          }
          // if lowest address count pc found, delete it, then allow to insert new entry
          if(returnPC!=-1)
          {
            //   deletion will be done after resetting hot-line for corresponding addresses
            //   table.erase(table.find(returnPC));
          }
    }
    else
    {
        it=table.find(pc);
        if(it!=table.end()){
            notfound=true;
            returnAddr=it->second->addEntry(addr);
        }
        else
        {
            table.insert({pc, new AddressHistory()});
        }
    }
}

IntPtr AddressHistory::addEntry(IntPtr addr)
{
    IntPtr retAddr=-1;
    bool notFound=true;
    if(!valid())
    {
        //do entry replacements
        // LRU
        UInt32 mN = 1e9;
        std::pair<UInt32,IntPtr> tmp;
        std::list<std::pair<UInt32,IntPtr>>::iterator it1;
        addrAndCount.sort();
        it1=addrAndCount.begin();
        cumulativeAccess=cumulativeAccess-tmp.first;
        retAddr=tmp.second;
        addrAndCount.erase(it1);
    }
    else
    {
        cumulativeAccess++;
        for(auto obj: addrAndCount)
        {
            if(obj.second == addr)
            {
                obj.first=obj.first+1;
                notFound=false;
                break;
            }
        }
    }

    if(notFound)
    {
        addrAndCount.push_back({1,addr});
    }
    return retAddr;// cacheblockinfo corresponding to these return address must be reset from hotline 
}


std::list<std::pair<UInt32,IntPtr>> PCHistoryTable::getAddress(IntPtr pc)
{
    std::list<std::pair<UInt32,IntPtr>> tmplist;
    std::map<IntPtr, AddressHistory*>::iterator it;
    it=table.find(pc);
    if(it!=table.end())
    {
        tmplist=it->second->getAddress();
    }
    return tmplist;
}