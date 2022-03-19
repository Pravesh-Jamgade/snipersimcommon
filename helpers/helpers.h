#ifndef HELPER_H
#define HELPER_H

#include<iostream>
#include "pqueue.h"
#include "fixed_types.h"


namespace Helper
{
    class PQueue
    {
        pqueue_t* pQ;
        public:
        PQueue(){
            // pQ=pqueue_init()
        }
        void insert();
        void peek();
        void remove();
        void update();
    };

    class Counter
    {
        UInt64 count;
        UInt64 reuse;
        public:
        Counter(int init=1){this->count=init, this->reuse=init;}
        void increase(){this->count++;}
        void increase(UInt64 byk){this->count=this->count+byk;}
        UInt64 getCount(){return this->count;}
        void decrease(){this->count--;}
        void decrease(UInt64 byk){this->count=this->count-byk;}
        void reset(){this->count=this->reuse;}
    };
}
#endif