#include "cache_addon.h"
#include<iostream>
#include<unordered_map>
#include "EpocHelper.h"
using namespace CacheAddonSpace;

// return top pc's addresses
std::unordered_set<IntPtr> PCHistoryTable::action(){
    std::unordered_set<IntPtr> allHots;
    std::map<IntPtr, AddressHistory> toptracker;

    // if(EpocHelper::getEpocStatus())
    // {
        // //erasing some pc entry
        // std::vector<std::pair<UInt64,IntPtr>> allPc;// count-pc
        // for(auto e: table)
        // {
        //     allPc.push_back({e.second.getTotalAccessCount(), e.first});
        // }
        // std::sort(allPc.begin(),allPc.end(), 
        // [](std::pair<UInt64, IntPtr> A, std::pair<UInt64, IntPtr>B){
        //     return A.first>B.first;
        // });

        // int size = allPc.size();
        // int top = size/2;

        // for(int i=0;top;i++)//count-pc
        // {
        //     auto it = table.find(allPc[i].second);//pc
        //     if(it!=table.end())
        //     {
        //         toptracker.emplace(it->first, it->second);
        //     }
        //     top--;
        // }

        // table=std::map<IntPtr, AddressHistory>();

        // if(toptracker.size()!=0){
        //     table.insert(toptracker.begin(), toptracker.end());
        // }

        // for(auto pc : table){
        //     for(auto addr: pc.second.getAddresses()){
        //         allHots.insert(addr.first);
        //     }
        // }
    // }
    
    // prevCycleNumber=modCycle;
    return allHots;
}

void PCHistoryTable::insert(IntPtr pc, IntPtr addr)
{
    auto findPC =table.find(pc);

    if(findPC!=table.end())
    {
        AddressHistory *addrh=&(findPC->second);
        bool pri = addrh->insert(addr);  //new or exixting entry
        if(pri==0){ //implies new entry of address, count addresses
        }
    }
    else 
    {
        table.emplace(pc,AddressHistory(addr));
    }
}

// returns address count if already address entry exists else returns 0 for new entry
bool AddressHistory::insert(IntPtr addr)
{
    increase(totalpcCounter);
    auto findAddr = addressCount.find(addr);
    
    if(findAddr!=addressCount.end()){
        findAddr->second++;
        return true; //exist
    }
    else
    {
        addressCount.emplace(addr, 1);
        return false; //new
    }
    
}