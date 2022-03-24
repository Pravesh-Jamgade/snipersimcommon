#include "cache_addon.h"
#include<iostream>
#include<unordered_map>
using namespace CacheAddonSpace;

void PCHistoryTable::insert(IntPtr pc, IntPtr addr)
{
    std::unordered_map<IntPtr,AddressHistory>::const_iterator const_it=table.find(pc);

    if(const_it!=table.end())
    {
        AddressHistory addrh=const_it->second;
        int pri = addrh.insert(addr);// will return frequency of addr, if 0->1st time and it is unique for pc hence also increase addressCounter
        if(pri==0)//implies new address
            addressCounter.increase();//only counting new addr
    }
    else 
    {
        table.insert({pc,AddressHistory(addr)});
        pcCounter.increase();// only counting new pc entry
    }
}

// returns address count if already address entry exists else returns 0 for new entry
UInt64 AddressHistory::insert(IntPtr addr)
{
    std::unordered_map<IntPtr, Helper::Counter*>::const_iterator const_it=addressCount.find(addr);
    Helper::Counter* counter = new Helper::Counter();
    if(const_it!=addressCount.end())
    {
        counter=const_it->second;
        counter->increase();
        return counter->getCount();
    }
    else
    {
        addressCount.insert({addr, counter});
        return 0;
    }
    
}