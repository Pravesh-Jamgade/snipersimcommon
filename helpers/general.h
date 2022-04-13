#include<map>
#include "fixed_types.h"
#include <stdio.h>      /* printf */
#include <unistd.h>
#include<sstream>

#include <fstream>
#include <iostream>
#include <unistd.h>
#include<stack>
#include<vector>
#include<memory>

namespace cache_helper
{
    class Misc
    {
        public:

        static String toHex(IntPtr val){
            std::stringstream ss;
            String hval;
            ss << std::hex << val; 
            ss >> hval; 
            ss.clear();
            return hval;
        }

        static void pathAppend(String& path, String name, bool isTLB=0)// tlb flag not needed anymore as i am not logging its access
        {
            String next = "->";
            path += name;
        }

        static void stateAppend(bool state, String& path)
        {
            String Hit, Miss;
            Hit=" (H)->"; Miss=" (M)->";
            path += state?Hit:Miss;
        }

        static void separatorAppend(String& path)
        {
            String sep = "#";
            path+=sep;
        }

        static void aliasAppend(String& path, String alias)
        {
            String open = "(";
            String close = ")";
            path+= open + alias + close;
        }

        static String collectPaths(String& path1, String& path2)
        {
            separatorAppend(path1);
            separatorAppend(path2);
            String space = ",";
            String open = "[";
            String close = "]";
            return open+ path1 + space + path2 +close;
        }

        static void pathAdd(bool state, MemComponent::component_t m_comp, String alias, String& path, bool enable=true)
        {
            String next = "->";
            path += MemComponent2String(m_comp);
            if(enable)
                aliasAppend(path, alias);
            stateAppend(state, path);
        }

        static bool accessTypeInfo(Core::mem_op_t mem_op_type)
        {
            bool result;
            switch (mem_op_type)
            {
                case Core::READ:
                case Core::READ_EX:
                    result = true;//LOAD
                    break;
                case Core::WRITE:
                    result = false;//STORE
                    break;
                default:
                    LOG_PRINT_ERROR("Unsupported Mem Op Type: %u", mem_op_type);
                    exit(0);
                    break;
            }
            return result;
        }

        static String AppendWithSpace()
        {
            return " " ;
        }

        template<typename P, typename ...Param>
        static String AppendWithSpace(const P &p, const Param& ...param)
        {
            String space = " ";
            String res = p+ space + AppendWithSpace(param...);
            return res;
        }
    };

};
