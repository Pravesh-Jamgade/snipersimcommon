#ifndef CACHE_HELPER
#define CACHE_HELPER

#include<map>
#include "fixed_types.h"

        #include <stdio.h>      /* printf */
        #include <unistd.h>
        #include "log.h"
        #include<sstream>

        #include <iostream>
        #include <filesystem>
        #include <unistd.h>
        #include<stack>
        #include<vector>
namespace cache_helper
{   

class Misc
{
    
    public:
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
};

 class DataInfo
{
    public:
    DataInfo(bool access_type, uint32_t stride){
        this->typeCount[access_type]+=1;
        this->stride.insert(stride);
        this->total_access=1;
    }
    uint32_t typeCount[2] = {0};
    std::set<uint32_t> stride;
    UInt32 total_access;
};

class StrideTable
{
    public:

    // ******accessinfo********

    // addr to datainfo
    typedef std::map<String, DataInfo*> Add2Data;

    // eip to access info
    std::map<String, Add2Data> table;

    UInt32 last=0;

    UInt32 total, reeip, readdr;

    std::map<String, std::set<String>> path2haddrStorage;

    std::map<String, std::vector<String>> countByName;

    StrideTable() { }

    ~StrideTable() {write();}

    void write()
    {
        std::map<String, Add2Data>::iterator pit = table.begin();
        std::set<uint32_t>::iterator sit;
        for(;pit!=table.end();pit++)
        {
            Add2Data add2data = pit->second;
            Add2Data::iterator qit = add2data.begin();
            
            for(;qit!=add2data.end(); qit++)
            {
                 if(qit->second)
                 {
                    DataInfo* dataInfo = qit->second;
                    _LOG_PRINT_CUSTOM(Log::Warning, "EIP=%s,ADDR=%s,LD=%ld,ST=%ld, T=%ld,STRIDE=[",
                    pit->first.c_str(), qit->first.c_str(), dataInfo->typeCount[1], dataInfo->typeCount[0], dataInfo->total_access);

                    sit = dataInfo->stride.begin();
                    for(;sit!=dataInfo->stride.end();sit++)
                    {
                        // _LOG_PRINT(Log::Warning, "%ld, ",dataInfo->stride[i]);
                        _LOG_PRINT_CUSTOM(Log::Warning, "%ld, ",sit);
                    }
                    // _LOG_PRINT(Log::Warning, "load=%ld store=%ld",dataInfo->typeCount[1], dataInfo->typeCount[0]);
                    _LOG_PRINT_CUSTOM(Log::Warning, "]\n");
                 }
            }
        }

        _LOG_PRINT_CUSTOM(Log::Warning, "*********************************PATH-2-ADDRESSES*******************************\n");

        std::map<String, std::set<String>>::iterator pasit = path2haddrStorage.begin();
        for(;pasit!=path2haddrStorage.end();pasit++)
        {
            _LOG_PRINT_CUSTOM(Log::Warning, "%s\t=[",pasit->first.c_str());
            std::set<String> addresses = pasit->second;
            // std::set<String>::iterator it = addresses.begin();
            for(auto address : addresses)
            {
                _LOG_PRINT_CUSTOM(Log::Warning, "%s,", address.c_str());
            }
            _LOG_PRINT_CUSTOM(Log::Warning, "]\n");
        }

        // printf("Total Access=%d\n", total);

        // printf("Total Access by Access Type\n");
        // std::map<String, std::vector<String>>::iterator icn = countByName.begin();
        // for(;icn!=countByName.end();icn++)
        // {
        //     String name = icn->first;
        //     char* p=&name[0];
        //     printf("%s = %d\n", p, icn->second.size());
        // }
        // icn = countByName.begin();
        // countByName.erase(icn, countByName.end());
    }

    void lookupAndUpdate(int access_type, IntPtr eip, IntPtr addr, String path)
    {   
        total++;
        // for stride calculation
        UInt32 tmp = addr;
        UInt32 diff = abs((long)tmp - last);

        // for indexing,
        IntPtr index = eip;

        std::stringstream ss;
        String hindex, haddr;
        ss << std::hex << index; ss >> hindex; ss.clear();
        ss << std::hex << addr; ss >> haddr; ss.clear();

        // std::cout<<hindex<<" "<<haddr<<" "<<htag<<" "<<name<<" "<<eip<<std::endl ;

        // iteratros
        std::map<String, Add2Data>::iterator it;
        Add2Data:: iterator ot;
        std::map<uint32_t, std::set<String>>:: iterator pt;
        std::set<String>:: iterator st;
        
        // store path if not exists already
        path2haddrStorage[path].insert(haddr);
        // if(path2haddrStorage.find(path)!=path2haddrStorage.end())
        //     path2haddrStorage[path].insert(haddr);
        // else
        //     path2haddrStorage.insert({path, {haddr}});

        // find eip on table
        it = table.find(hindex);
        // if eip found
        if(it!=table.end())
        {
            reeip++;

            // get accessinfo for correspnding eip
            Add2Data *add2data = &it->second;

            // find if addr is seen before, find corresponding data info
            ot = add2data->find(haddr);

            // if accessinfo found / addr seen before
            if(ot!=add2data->end())
            {
                readdr++;

                // get datainfo
                DataInfo* dataInfo = ot->second;
                // update datainfo
                dataInfo->typeCount[access_type]+=1;
                dataInfo->total_access++;
                dataInfo->stride.insert(diff);
            }
            else // eip seeen before but addr is new
            {
                add2data->insert( 
                        { haddr, new DataInfo(access_type, diff, path) } 
                );
            }
        }
        else
        {
            table[hindex][haddr]=new DataInfo(access_type, diff, path);
        }

        last = tmp;

        std::map<String, std::vector<String>>::iterator icn = countByName.find(name);
        if(icn!=countByName.end())
        {
            icn->second.push_back(haddr);
        }
        else{
            countByName.insert({name, {haddr}});
        }
    }
};

class Access
{
    IntPtr eip, addr;
    String objectName;
    bool instAccessType;// st=0 ld=1
    public:
    Access(IntPtr eip, IntPtr addr, String objectName, bool accessType){
        this->eip=eip; this->addr=addr; this->objectName=objectName; thhis->instAccessType=accessType;
    }

    String getObjectName(){return objectName;}
    IntPtr getEip(){return eip;}
    bool getAccessType(){return this->instAccessType;}
    IntPtr getAddr(){return this->addr;}
};

class CacheHelper
{
    std::stack<Access*> request;
    StrideTable strideTable;
    UInt32 cacheBlockSize;
    bool memAccessType;// cache=0 dram=1
    public:
    
    void addRequest(IntPtr eip, IntPtr addr, String objname, Cache* cache, Core::mem_op_t mem_op_type){
        bool accessType = accessTypeInfo(mem_op_type);
        //todo: i can take some contant to generate these inforamtion regardless of where access is comming from
        if(cache==NULL)
        {
            // for dram access, i dont know how to mask it yet;
        }
        else
        {
            // for cache access, i am taking into account cache size, blocksize and calculating indexing information
            IntPtr tag;
            UInt32 set_index;
            UInt32 block_offset;
            cache->splitAddress(addr, &tag, &set_index, &block_offset)

            IntPtr eip_tag;
            UInt32 eip_set_index;
            UInt32 eip_block_offset;
            cache->splitAddress(eip, eip_tag, eip_set_index, eip_block_offset);

            IntPtr index = eip & 0xfffff; // use lsb 20 bits for indexing, possible all instructions are in few blocks
            IntPtr blockBasedStride = addr >> cache->getLogBlockSize() & cache->getBlockMask(); // as we want to calculate stride beyond a single block
            IntPtr setBasedStride = addr >> cache->getLogBlockSize() & cache->getSetMask();
        }
        request.push(new Access(eip,addr,objname,accessType));
    }
    void addRequest(Access* access){request.push(access);}
    void setCacheBlockSize(UInt32 cache_block_size){this->cacheBlockSize=cache_block_size;}
    
    bool accessTypeInfo(Core::mem_op_t mem_op_type)
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
    void strideTableUpdate()
    {
        while(!request.empty())
        {   
            Access* access = request.top(); 
            request.pop();
            char* p=&access->getObjectName()[0];
            strideTable.lookupAndUpdate(access->getAccessType(), access->getEip(), access->getAddr(), access->getObjectName());
        }
    }
};

};
#endif