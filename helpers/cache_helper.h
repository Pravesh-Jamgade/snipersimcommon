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
namespace cache_helper
{   

class PathStore
{
    static std::map<IntPtr, String> pathStore;
    public:
    bool find(IntPtr eip)
    {
        return pathStore.find(eip)!= pathStore.end();
    }
};

class Sampling
{
    public:
    bool allowed;
    int sampleLimit;
    int provided;
    Sampling(int sampleLimit=100000, bool flag=false){
        this->sampleLimit = sampleLimit;
        provided = sampleLimit;
        allowed=flag;
    }

    void flip(){
        allowed!=allowed;
    }

    void reset(){
        sampleLimit=provided;
    }

    void count(){
        if(sampleLimit==0){
            flip();
            reset();
            return;
        }
        sampleLimit--;
    }

};

class Misc
{
    public:
    static void pathAppend(String& path, String name, bool isTLB=0)// tlb flag not needed anymore as i am not logging its access
    {
        String next = "->";
        String wait = "*";// waiting to know if accessed block is null or not. which implies miss/hit.
        
        path += name;
        // if(isTLB){
        //     path = path + name + wait;
        // }
        // else{
        //     path = path+name+next;
        // }
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

    static String collectPaths(String& path1, String& path2)
    {
        separatorAppend(path1);
        separatorAppend(path2);
        String space = ",";
        String open = "[";
        String close = "]";
        return open+ path1 + space + path2 +close;
    }
};

 class DataInfo
{
    public:
    DataInfo(bool access_type, uint32_t stride, String tag, String name){
        this->typeCount[access_type]+=1;
        this->tag = tag;
        this->stride.push_back(stride);
        this->total_access=1;
        this->name = name;
    }
    uint32_t typeCount[2] = {0};
    std::vector<uint32_t> stride;
    std::vector<String> paths;
    String tag;
    UInt32 total_access;
    String name;
};


class StrideTable
{
    public:
    StrideTable()
    { }

    ~StrideTable()
    {
        write();
    }

    // accessinfo
    // addr to datainfo
    typedef std::map<String, DataInfo*> Add2Data;
    // eip to access info
    std::map<String, Add2Data> table;


    UInt32 last=0;
    int total, reeip, readdr;
    
    void write()
    {
        std::map<String, Add2Data>::iterator pit = table.begin();
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

                    for(int i=0; i< dataInfo->stride.size(); i++)
                    {
                        // _LOG_PRINT(Log::Warning, "%ld, ",dataInfo->stride[i]);
                        _LOG_PRINT_CUSTOM(Log::Warning, "%ld, ",dataInfo->stride[i]);
                    }
                    // _LOG_PRINT(Log::Warning, "load=%ld store=%ld",dataInfo->typeCount[1], dataInfo->typeCount[0]);
                    _LOG_PRINT_CUSTOM(Log::Warning, "]\n");
                 }
            }
        }
    }

    void lookupAndUpdate(int access_type, IntPtr eip, IntPtr addr, 
    IntPtr tag, UInt32 set_index, String name)
    {   
        total++;
        // for stride calculation
        UInt32 tmp = addr;
        UInt32 diff = abs((long)tmp - last);

        // for indexing,
        IntPtr index = eip;

        std::stringstream ss;
        String hindex, haddr, htag;
        ss << std::hex << index; ss >> hindex; ss.clear();
        ss << std::hex << addr; ss >> haddr; ss.clear();
        ss << std::hex << tag; ss >> htag;

        // std::cout<<hindex<<" "<<haddr<<" "<<htag<<" "<<name<<" "<<eip<<std::endl ;

        // iteratros
        std::map<String, Add2Data>::iterator it;
        Add2Data:: iterator ot;

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
                dataInfo->tag = htag;
                dataInfo->typeCount[access_type]+=1;
                if(dataInfo->stride.back() != diff)
                {
                    dataInfo->stride.push_back(diff);
                }
                dataInfo->total_access++;
            }
            else // eip seeen before but addr is new
            {
                add2data->insert( 
                        {haddr, new DataInfo(access_type, diff, htag, name) } 
                );
            }
        }
        else
        {
            table[hindex][haddr]=new DataInfo(access_type,  diff, htag, name);
        }

        last = tmp;
    }
};

class CacheHelper
{
    public:
    // helper tools

    // member variable
    StrideTable stride_table;
    Sampling sampling;

    // member function
    void strideTableUpdate(bool access, IntPtr eip, IntPtr addr, IntPtr tag, UInt32 set_index, String name, IntPtr origEip)
    {
        char* p=&name[0];
        printf("%ld = %s\n", origEip,p);

        stride_table.lookupAndUpdate(access, eip, addr, tag, set_index, name);
        sampling.count();
    }
};

};
#endif