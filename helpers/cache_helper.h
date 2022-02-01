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
        #include <memory>

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
};

class StrideOrder
{
    String address;
    uint32_t count;
    public:
    StrideOrder(String addr):address(addr), count(1){} 

    bool operator==(StrideOrder pre) {return address==pre.address;}
    void incrCount() {count++;}
    String getAddress(){return address;}
    uint32_t getAddrCount(){return count;}
};

/*Data regarding access pattern of particular eip, stroes stride seen by eip on behalf of addresses,
 (StrideOrder) access order of addresses and times the address has been accessed*/
class StrideCluster
{
    std::set<String> strideList;//tracks only stride
    std::vector<StrideOrder> strideOrderList;//tracks addresses responsible for stride
    IntPtr lastSeenAddr=0;//tracks last seen address, to calculate stride for next access
    public:
    StrideCluster(){}
    StrideCluster(IntPtr initAddr, String haddr){ 
        String initAddrStr = itostr(initAddr);
        strideList.insert(initAddrStr); //stride from incoming address
        StrideOrder newStrideOrderElement(haddr); // create new strideOrder and track it, in hex form
        strideOrderList.push_back(newStrideOrderElement);//incoming address order
        lastSeenAddr=initAddr; 
    }
    ~StrideCluster(){};
    
    IntPtr getLastSeenAddr() { return this->lastSeenAddr;}
    void setStride(IntPtr newAddr, String haddr) { 
        IntPtr diff = newAddr-lastSeenAddr;
        if(diff<0)
            diff=-1*diff; 
        
        lastSeenAddr=newAddr;

        String diffStr = itostr(diff);
        strideList.insert(diffStr); 

        StrideOrder newstrideOrderElement(haddr);
        if(newstrideOrderElement==strideOrderList.back()){
            strideOrderList.back().incrCount();
            return;}//if prev strideorder == newstrideorder skip, otherwise track
        strideOrderList.push_back(newstrideOrderElement);
    }
    std::set<String> getStrideList(){return strideList;}
    std::vector<StrideOrder> getStrideOrder(){return strideOrderList;}
};

 class DataInfo
{
    UInt32 typeCount[2] = {0};
    UInt32 total_access;

    public:
    DataInfo(bool access_type){
        this->typeCount[access_type]+=1;
        this->total_access=1;
    }
    void incrTotalCount(){total_access++;}
    void incrTypeCount(int accessType){typeCount[accessType]+=1;}
    UInt32 getCount(int type){ if(type<0){return total_access;}else{return typeCount[type];} }
};

class StrideTable
{
    public:

    // ******accessinfo********

    // addr to datainfo
    typedef std::map<String, DataInfo*> Add2Data;

    //* eip to access info
    std::map<String, Add2Data> table;

    //
    std::map<String, StrideCluster> strideClusterInfo;

    std::map<String, UInt64> eipFreq; // eip  frequcny

    UInt32 last=0;

    UInt32 total=0,reeip=0,readdr=0;

    //* path(aka name of structure) to  requested address
    std::map<String, std::set<String>> path2haddrStorageInfo;

    // path(ake name of structure) to count of requested addresses
    std::map<String, UInt32> countByNameInfo;

    //* eip address cycle#
    std::vector<String> cycleInfo;
    String cycleInfoOutput="/cycleStat.out";
    String strideAddrOrderOutput="/strideOrderStat.out";

    String outputDirName;
    
    StrideTable() {}

    ~StrideTable() {}

    void lookupAndUpdate(int access_type, IntPtr eip, IntPtr addr, String path, UInt64 cycleNumber, bool accessResult);
    void write();
    void setOutputDir(String outputDir){this->outputDirName=outputDir;}
};

class Access
{
    IntPtr eip, addr;
    String objectName;
    UInt64 cycleCount;
    bool instAccessType;// st=0 ld=1
    bool accessResult;
    public:
    Access(IntPtr eip, IntPtr addr, String objectName, UInt64 cycleCount, bool accessType, bool accessResult){
        this->eip=eip; this->addr=addr; this->objectName=objectName; this->instAccessType=accessType;
        this->cycleCount=cycleCount; this->accessResult=accessResult;
    }

    String getObjectName(){return objectName;}
    IntPtr getEip(){return eip;}
    bool getAccessType(){return this->instAccessType;}
    IntPtr getAddr(){return this->addr;}
    UInt64 getCycleCount(){return this->cycleCount;}
    bool getAccessResult(){return this->accessResult;}
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
    void addRequest(IntPtr eip, IntPtr addr, String objname, Cache* cache, UInt64 cycleCount, bool accessType, bool accessResult);
    std::stack<Access*> getRequestStack(){return this->request;}
    void setOutputDir(String outputDir){ strideTable.setOutputDir(outputDir); }
};

};
#endif