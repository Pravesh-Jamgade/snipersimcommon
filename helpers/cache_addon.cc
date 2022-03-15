#include "cache_addon.h"
#include "pqueue.h"
#include <algorithm>
#include <map>

using namespace CacheAddonSpace;

void PCHistoryTable::insert(IntPtr pc, IntPtr addr, IntPtr& returnPC, IntPtr& returnAddr){
    std::map<IntPtr, AddressHistory*>::iterator it;
    returnPC=returnAddr=-1;
    bool notfound=true;
    if(valid())
    {
        // maintain priority queue
        addEntry(pc);

        // maintain table structure 
        it=table.find(pc);
        if(it!=table.end()){
            notfound=false;
            returnAddr=it->second->insert(addr);
        }
        else
        {
            table.insert({pc, new AddressHistory()});
        }
    }
    else
    {
        // replacement
        // LFU

        // less freuent node
        node_t *ns=(node_t*)pqueue_peek(pq);//remove min element, as pri is negative value. less frq pc will be at top
        
        returnPC=ns->val;// pc
        //reset less frequent pc
        priority[ns->pos]=0;// at pos index reset as we have removed these entry from priority queue
        pcStore[ns->pos]=0;
        validTableEntry[ns->pos]=false;
        pqueue_remove(pq,ns);
    }
}

IntPtr* PCHistoryTable::getAddress(IntPtr pc)
{
    std::map<IntPtr, AddressHistory*>::iterator it;
    it=table.find(pc);
    if(it!=table.end())
    {
        return it->second->getAddress();
    }
}