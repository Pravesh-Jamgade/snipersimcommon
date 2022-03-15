#ifndef CACHE_ADDON
#define CACHE_ADDON

#include "fixed_types.h"
#include <iostream>
#include <vector>
#include <map>
#include <list>
#include "pqueue.h"

namespace CacheAddonSpace
{   
    typedef struct node_t
    {
        pqueue_pri_t pri;
        int    val;
        size_t pos;
    } node_t;

    class Function
    {
        public: 
        static int
        cmp_pri(pqueue_pri_t next, pqueue_pri_t curr)
        {
            return (next < curr);
        }


        static pqueue_pri_t
        get_pri(void *a)
        {
            return ((node_t *) a)->pri;
        }


        static void
        set_pri(void *a, pqueue_pri_t pri)
        {
            ((node_t *) a)->pri = pri;
        }


        static size_t
        get_pos(void *a)
        {
            return ((node_t *) a)->pos;
        }


        static void
        set_pos(void *a, size_t pos)
        {
            ((node_t *) a)->pos = pos;
        }
    };

    class AddressHistory
    {
        // count - address
        pqueue_t *pq;
        node_t *ns; 
        int currSize;
        int entryLimit;
        IntPtr addrStore[10]; // address
        UInt32 priority[10]={0};  // address count as priority
        bool validEntry[10]={false};

        void addEntry(IntPtr addr){
            bool notfound=true;
            int i=0;
            for(;i< entryLimit; i++)// check if already exists
            {
                if(!validEntry[i])
                    continue;
                if(addrStore[i]==addr)
                {
                    priority[i]--;

                    // found index of addr now increase count in priority queue
                    ns =(node_t*)malloc(sizeof(node_t));
                    ns->pri=priority[i];
                    ns->val=i;
                    pqueue_incr_priority(pq, &ns);

                    notfound=false;
                    break;
                }
            }

            if(notfound)//if not already exists
            {
                addrStore[i]=addr;
                priority[i]=-1;
                validEntry[i]=true;
                
                ns=(node_t*)malloc(sizeof(node_t));
                ns->pri=-1;// intialize priority for new entry to 1
                ns->val=i;// val is our array index
                pqueue_insert(pq, &ns);
            }
        }
      
        public:

        AddressHistory()
        {
            currSize=0;
            entryLimit=10;
            pq=pqueue_init(10, Function::cmp_pri, Function::get_pri, Function::set_pri, Function::get_pos, Function::set_pos);
        }

        ~AddressHistory()
        {
            delete pq;
            delete ns;
        }

        IntPtr insert(IntPtr addr){
            IntPtr retAddr=-1;
            if(valid())
            {
                currSize++;
                addEntry(addr);
            }
            else{
                currSize--;
                //apply replacement on address entries
                // LFU
                ns =(node_t*)malloc(sizeof(node_t));
                ns=(node_t*)pqueue_peek(pq);//remove min element

                retAddr=addrStore[ns->val];
                //reset
                priority[ns->val]=0;
                addrStore[ns->val]=0;
                validEntry[ns->val]=false;
                //remove
                pqueue_remove(pq,ns);
            }
            return retAddr;
        }

        bool valid(){ return pq->size < entryLimit;}
        int getEntrySize(){ return pq->size;}

        IntPtr* getAddress(){
            return addrStore;
        }
    };

    class PCHistoryTable
    {
        // pc - list(pair(count - address))
        std::map<IntPtr, AddressHistory*> table;
        int tableLimit;
        IntPtr pcStore[500];  // pc
        UInt32 priority[500]={0}; // pc count as priority
        bool validTableEntry[500]={false};

        void addEntry(IntPtr pc){
            bool notfound=true;
            int i=0;
            for(;i< tableLimit; i++)
            {
                if(!validTableEntry[i])
                    continue;
                if(pcStore[i]==pc)
                {
                    priority[i]--;

                    // found index of addr now increase count in priority queue
                    ns =(node_t*)malloc(sizeof(node_t));
                    ns->pri=priority[i];
                    ns->val=pc;
                    ns->pos=i;

                    pqueue_incr_priority(pq, &ns);

                    notfound=false;
                    break;
                }
            }

            if(notfound)
            {
                pcStore[i]=pc;
                priority[i]=-1;
                validTableEntry[i]=true;
                
                ns=(node_t*)malloc(sizeof(node_t));
                ns->val=pc;// val is our array index
                ns->pri=-1;// intialize priority for new entry to 1
                ns->pos=i;
                pqueue_insert(pq, &ns);
            }
        }

        public:
        pqueue_t *pq;
        node_t *ns;
        
        PCHistoryTable(){
            tableLimit=500;
            pq=pqueue_init(500, Function::cmp_pri, Function::get_pri, Function::set_pri, Function::get_pos, Function::set_pos);
            ns=(node_t*)malloc(sizeof(node_t));
        }
        ~PCHistoryTable(){
            delete pq;
            delete ns;
        }

        bool valid(){ return pq->size<tableLimit;}

        void insert(IntPtr pc, IntPtr addr, IntPtr& retPC, IntPtr& retAddr);

        IntPtr* getAddress(IntPtr pc);

        void eraseEntry(IntPtr pc){
            std::map<IntPtr, AddressHistory*>::iterator it = table.find(pc);
            if(it!=table.end()){
                table.erase(pc);
            }
        }
        
        int tableEntrySize(IntPtr pc, IntPtr addr){
            if(table[pc])
            {
                table[pc]->getEntrySize();
            }
        }
        int tableSize(){return table.size();}
    };
};
#endif