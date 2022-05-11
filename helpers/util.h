#ifndef UTIL_H
#define UTIL_H
namespace Util
{
    class Misc{
        public:
        static String toHex(IntPtr val){
            std::stringstream* ss = new std::stringstream;
            String hval;
            (*ss) << std::hex << val; 
            (*ss) >> hval; 
            std::stringstream().swap(*ss);
            return hval;
        }
    };
}
#endif UTIL_H