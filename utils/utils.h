#ifndef UTILS_H
#define UTILS_H

namespace utilspace
{
    class Counter
    {   
        UInt64 count;
        public:
        Counter(UInt64 x=0){
            count=x;
        }
        void increase(int k=1){count+=k;}
        UInt64 getCount(){return count;}
    };
    
} // namespace name

#endif