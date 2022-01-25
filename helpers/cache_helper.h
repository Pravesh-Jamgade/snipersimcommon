#ifndef CACHE_HELPER
#define CACHE_HELPER

#include<map>
#include "fixed_types.h"
#include "cache.h"
        #include <stdio.h>      /* printf */
        #include <unistd.h>
        #include "log.h"
        #include<sstream>

        #include <fstream>
        #include <iostream>
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

    static String AccessLocationByPostionBit(SInt8 location)
    {
        switch(location)
        {
            case 1: return "dram-cntrl";
            case 2: return "dram-cache";
            case 4: return "L3";
            case 8: return "L2";
            case 16: return "L1-D";
            case 32: return "L1-I";
            case 64: return "dtlb";
            case 128: return "itlb";
            case 256: return "stlb";
        }
        return "*location*";
    }

    /*
    Important: will try to store flags or names of memory element in bits access them in following way: to reduce application data
        bit nuumber        | 10            |                  9|   8|   7|  6|  5| 4| 3|2|1|                 0|
        bit description    |AccessType(L/S)|AccessLocation(stlb|itlb|dtlb|l1i|l1d|l2|l3|D|D)|AccessStatus(h/m)|
        # bits used        |        1 bit  |                        8 bit                 |       1 bit       |
    */
    
    static void RetriveFlags(SInt16 data, String& access_status, String& access_type, String& access_location)
    {   
       
        if(data & 1) // checking lsb bit 
            access_status="h";
        else 
            access_status="m";
        
        SInt8 location = data>>1 & ((1<<9) - 1); // need middle 9 bits, hence 9 bit mask for finding memory accessed 

        access_location = AccessLocationByPostionBit(location);// getting memory name

        if(data >> 10 & 1) // 11th bit, shift right 10 bits and mask 1 bit
            access_type = "L";
        else access_type = "S";
    }

    static void SetHitFlag(SInt16& data){ data=data|1;} // last lsb bit is h/m bit
    static void SetAccessTypeFlag(SInt16& data){ data= data | 1<<10; } // last msb bit is L/S bit
    static void SetAccessLocationFlag(SInt16& data, String name) // from name apply bit mask for that position
    { 
        if(name=="stlb")
        {
            data=data | 1<<9; 
        }
        if(name=="itlb")
        {
            data=data | 1<<8; 
        }
        if(name=="dtlb")
        {
            data=data | 1<<7; 
        }
        if(name=="L1-I")
        {
            data=data | 1<<6; 
        }
        if(name=="L1-D")
        {
            data=data | 1<<5; 
        }
        if(name=="L2")
        {
            data=data | 1<<4; 
        }
        if(name=="L3")
        {
            data=data | 1<<3; 
        }
        if(name=="dram-cache")
        {
            data=data | 1<<2; 
        }
        if(name=="dram-cntrl")
        {
            data=data | 1<<1; 
        }
    }

    static void PiggyBackStatus(SInt16& statusBits, String accessLocation, bool accessType, bool accessRes)
    {
        if(accessRes)// if hit then we set
            SetHitFlag(statusBits);
        if(accessType)// if load then we set
            SetAccessTypeFlag(statusBits);
        SetAccessLocationFlag(statusBits, accessLocation); // where access at least come dont bother about h/m
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

    //* eip to access info
    std::map<String, Add2Data> table;

    UInt32 last=0;

    UInt32 total, reeip, readdr;

    //* path(aka name of structure) to  requested address
    std::map<String, std::set<String>> path2haddrStorage;

    // path(ake name of structure) to count of requested addresses
    std::map<String, UInt32> countByName;

    //* eip address cycle#
    std::vector<String> cycleInfo;
    String cycleInfoOutput;
    
    StrideTable() {}

    ~StrideTable() {}

    void lookupAndUpdate(int access_type, IntPtr eip, IntPtr addr, String path, UInt64 cycleNumber);
    void write();
    void setOutputDir(String outputDir){this->cycleInfoOutput=outputDir+"/cycleStat.out";}

};

class Access
{
    IntPtr eip, addr;
    String objectName;
    UInt64 cycleCount;
    bool instAccessType;// st=0 ld=1
    SInt16 piggyBack;
    public:
    Access(IntPtr eip, IntPtr addr, String objectName, UInt64 cycleCount, bool accessType, SInt16 piggyBack){
        this->eip=eip; this->addr=addr; this->objectName=objectName; this->instAccessType=accessType;
        this->cycleCount=cycleCount; this->piggyBack=piggyBack;
    }

    String getObjectName(){return objectName;}
    IntPtr getEip(){return eip;}
    bool getAccessType(){return this->instAccessType;}
    IntPtr getAddr(){return this->addr;}
    UInt64 getCycleCount(){return this->cycleCount;}
    SInt16 getPiggyBack(){return this->piggyBack;}
};

class CacheHelper
{
    std::stack<Access*> request;
    StrideTable strideTable;
    bool memAccessType;// cache=0 dram=1
   
    public:
    void writeOutput(){ strideTable.write();}
    void strideTableUpdate();
    void addRequest(Access* access);
    void addRequest(IntPtr eip, IntPtr addr, String objname, Cache* cache, UInt64 cycleCount, bool accessType, SInt16 piggyBack=0);
    std::stack<Access*> getRequestStack(){return this->request;}
    void setOutputDir(String outputDir){ strideTable.setOutputDir(outputDir); }
};

};
#endif