#ifndef CACHE_ADDON
#define CACHE_ADDON

#include "fixed_types.h"
#include <iostream>
#include <unordered_map>
#include "pqueue.h"
#include "lock.h"

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

    class Counter
    {
        UInt64 count;
        public:
        Counter(int init=1){this->count=count;}
        void increase(){count++;}
        void increase(UInt64 byk){count=count+byk;}
        UInt64 getCount(){return count;}
        void decrease(){count--;}
        void decrease(UInt64 byk){count=count-byk;}
    };

    class AddressHistory
    {
        std::unordered_map<IntPtr,Counter> addressCount;
        public:
        AddressHistory(IntPtr addr){
            addressCount[addr]=Counter();
        }
        bool insert(IntPtr addr);// when true then only count; counting for new memory address only
    };

    class PCHistoryTable
    {
        Counter addressCounter;
        Counter pcCounter;
        std::unordered_map<IntPtr, AddressHistory> table;
        Lock *lock;
        public:
        PCHistoryTable(){
            lock = new Lock();
        }
        void insert(IntPtr pc, IntPtr addr);
        UInt64 getPCCount(){return pcCounter.getCount();}
        UInt64 getAddrCount(){return addressCounter.getCount();}
    };
};
#endif