#ifndef UTIL_H
#define UTIL_H
namespace Util
{
    class Misc{
        public:

        static String toHex(IntPtr val){
            std::stringstream ss;
            String hval;
            ss << std::hex << val; 
            ss >> hval; 
            ss.clear();
            return hval;
        }
    };
}
#endif UTIL_H