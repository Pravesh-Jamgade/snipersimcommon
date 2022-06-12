#include "cache_addon.h"
#include<iostream>
#include<unordered_map>
using namespace CacheAddonSpace;

void PCHistoryTable::insert(IntPtr pc, IntPtr addr)
{
    auto findPC =table.find(pc);

    if(findPC!=table.end())
    {
        AddressHistory *addrh=&(findPC->second);
        int pri = addrh->insert(addr);// will return frequency of addr, if 0->1st time and it is unique for pc hence also increase addressCounter
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
    totalpcCounter.increase();
    Helper::Counter* counter = find(addr);
    if(counter!=nullptr)
    {
        counter->increase();
        return counter->getCount();
    }
    else
    {
        addressCount.insert({addr, new Helper::Counter(1)});
        return 0;
    }
    
}