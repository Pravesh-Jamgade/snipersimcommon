#ifndef HELPER_H
#define HELPER_H

#include<iostream>
#include<vector>
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
        IntPtr hint;

        public:
        
        Counter(int init=1){this->count=init, this->reuse=init;}
        void increase(){this->count++;}
        void increase(UInt64 byk){this->count=this->count+byk;}
        UInt64 getCount(){return this->count;}
        void decrease(){this->count--;}
        void decrease(UInt64 byk){this->count=this->count-byk;}
        void reset(){this->count=this->reuse;}
    };

    class Message
    {
        public:
        int core_id;
        double miss_ratio;
        String source;
        UInt64 totalAccess, totalMiss;
        Message(int core_id, double miss_ratio, String source, UInt64 totalAccess, UInt64 totalMiss):
        core_id(core_id), miss_ratio(miss_ratio), 
        source(source),totalAccess(totalAccess), totalMiss(totalMiss)
        {}
        int getCore(){return core_id;}
        double getMissRatio(){return miss_ratio;}
        String getName(){return source;}
        UInt64 gettotalMiss(){return totalMiss;}
        UInt64 gettotalAccess(){return totalAccess;}
        double getMiss2HitRatio(){return (double)totalMiss/ (double)(totalAccess-totalMiss);}
    };

    class MsgCollector
    {
        std::vector<Message> messages;
        public:
        void push(Message msg){messages.push_back(msg);}
        std::vector<Message> getMsg(){return messages;}
    };

    class LevelPredictor
    {   
        std::vector<int>levelToSkip;
        public:
        LevelPredictor(int size)
        {
            levelToSkip=std::vector<int>(size,0);
        }
        void addLevelMiss(int level)
        {
            levelToSkip[level]=1;
        }
        void oredLevelMiss(int level)
        {
            levelToSkip[level]=levelToSkip[level]==1?1:0;
        }
        std::vector<int> getLevelPred(){return levelToSkip;}
    };
}
#endif