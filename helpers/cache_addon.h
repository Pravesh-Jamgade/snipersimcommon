#ifndef CACHE_ADDON
#define CACHE_ADDON

#include "fixed_types.h"
#include <iostream>
#include <unordered_map>
#include "pqueue.h"
#include "lock.h"
#include "helpers.h"

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
        std::unordered_map<IntPtr,Helper::Counter> addressCount;
        public:
        AddressHistory(IntPtr addr){
            addressCount[addr]=Helper::Counter();
        }
        bool insert(IntPtr addr);// true; counting for new entries of memory address only
    };

    class PCMisc
    {
        public:
        Helper::Counter addressCounter;
        Helper::Counter pcCounter;
        Helper::Counter refreshCounter;
        UInt64 prevCycleNumber
        PCMisc(){
            refreshCounter=Helper::Counter(10000);
        }
        UInt64 getPCCount(){return pcCounter.getCount();}//keep count of new pc only
        UInt64 getAddrCount(){return addressCounter.getCount();}//keep count of new addresses only
        virtual void action(UInt64 currCycleNumber)=0;
    };

    class PCHistoryTable: virtual public PCMisc
    {
        std::unordered_map<IntPtr, AddressHistory> table;
        Lock *lock;
        public:
        PCHistoryTable(){
            lock = new Lock();
        }
        void insert(IntPtr pc, IntPtr addr);
        void action(UInt64 currCycleNumber){
            prevCycleNumber=prevCycleNumber-currCycleNumber;
            refreshCounter.decrease(prevCycleNumber);
            prevCycleNumber=currCycleNumber;
            if(refreshCounter.getCount()<=0)
            {
                table.erase(table.begin(), table.end());
            }
        }
    };
};
#endif