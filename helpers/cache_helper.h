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
        #include<memory>
        #include<sqlite3.h>

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

class Storage
{
    const char* fileName = "TaceDB.db";//db name
    const char* sql_create_tables[10]={
        "CREATE TABLE `CycleBasedTrace` (type TEXT,status TEXT,object TEXT,pc TEXT,address TEXT,cycle TEXT) ",
        "CREATE TABLE `PCBasedCluster` (pc TEXT,address TEXT,count INTEGER)",
        "CREATE TABLE `PCBasedClusterStride` (pc TEXT,stride TEXT)",
        "CREATE TABLE `AddressFrequency` (address TEXT,frequency INTGER,cluster INTEGER)",
        "CREATE TABLE `AddressBasedCluster` (cluster INTEGER, size INTEGER, address TEXT)" 
    };

    const char* sql_insert_into[10]={
        "INSERT INTO `CycleBasedTrace`(type,status,object,pc,address,cycle) VALUES(?,?,?,?,?,?)",
        "INSERT INTO `PCBasedCluster`(pc,address,count) VALUES(?,?,?)",
        "INSERT INTO `PCBasedClusterStride`(pc,stride) VALUES(?,?)",
        "INSERT INTO `AddressFrequency` (address, frequency, cluster) VALUES(?,?,?)",
        "INSERT INTO `AddressBasedCluster` (cluster, size, address) VALUES(?,?,?)"
    };

    sqlite3 *db;
    
    sqlite3_stmt *sql_insert_cyclebased_trace, *sql_insert_pcbased_cluster;
    sqlite3_stmt *sql_insert_pcbased_stride, *sql_insert_addr_fre;
    sqlite3_stmt *sql_insert_addrbased_cluster;

    public:
    Storage(){};
    ~Storage(){
        if (db)
        {
            sqlite3_close(db);
        }
    }

    void open(){
        int ret = sqlite3_open(fileName,&db);
        LOG_ASSERT_ERROR(ret == SQLITE_OK, "Cannot create DB");
    }
    void createTables(){
        for(int i=0;i< sizeof(sql_create_tables)/sizeof(sql_create_tables[0]); i++)
        {
            int res; char* err;
            res = sqlite3_exec(db, sql_create_tables[i], NULL, NULL, &err);
            LOG_ASSERT_ERROR(res == SQLITE_OK, "Error executing create tables SQL statement \"%s\": %s", sql_create_tables[i], err);
        }
    }

    void startBind()
    {
        int res = sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
        LOG_ASSERT_ERROR(res == SQLITE_OK, "Error executing StartBind SQL statement: %s", sqlite3_errmsg(db));
    }

    void endBind()
    {
        int res = sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
        LOG_ASSERT_ERROR(res == SQLITE_OK, "Error executing EndBind SQL statement: %s", sqlite3_errmsg(db));
    }

    void bindCycleBasedTrace(String type,String status,String object,String pc,String addr,String cycle)
    {
        sqlite3_prepare(db, sql_insert_into[0], -1, &sql_insert_cyclebased_trace, NULL);
        startBind();
        sqlite3_reset(sql_insert_cyclebased_trace);
        sqlite3_bind_text(sql_insert_cyclebased_trace, 1, type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(sql_insert_cyclebased_trace, 2, status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(sql_insert_cyclebased_trace, 3, object.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(sql_insert_cyclebased_trace, 4, pc.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(sql_insert_cyclebased_trace, 5, addr.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(sql_insert_cyclebased_trace, 6, cycle.c_str(), -1, SQLITE_TRANSIENT);
        int res = sqlite3_step(sql_insert_cyclebased_trace);
        LOG_ASSERT_ERROR(res==SQLITE_DONE, "ERROR exexuting SQL statement %s", sqlite3_errmsg(db));
        endBind();
    }

    void  bindPCBasedCluster(String pc, String addr, int count)
    {
        sqlite3_prepare(db, sql_insert_into[1], -1, &sql_insert_pcbased_cluster, NULL);
        startBind();
        sqlite3_reset(sql_insert_pcbased_cluster);
        sqlite3_bind_text(sql_insert_pcbased_cluster, 1, pc.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(sql_insert_pcbased_cluster, 2, addr.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(sql_insert_pcbased_cluster,3,count);
        int res = sqlite3_step(sql_insert_pcbased_cluster);
        LOG_ASSERT_ERROR(res==SQLITE_DONE, "ERROR exexuting SQL statement %s", sqlite3_errmsg(db));
        endBind();
    }

    void bindPCBasedStride(String pc, String stride)
    {
        sqlite3_prepare(db, sql_insert_into[2], -1, &sql_insert_pcbased_stride, NULL);
        startBind();
        sqlite3_reset(sql_insert_pcbased_stride);
        sqlite3_bind_text(sql_insert_pcbased_stride, 1, pc.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(sql_insert_pcbased_stride, 2, stride.c_str(), -1, SQLITE_TRANSIENT);
        int res = sqlite3_step(sql_insert_pcbased_stride);
        LOG_ASSERT_ERROR(res==SQLITE_DONE, "ERROR exexuting SQL statement %s", sqlite3_errmsg(db));
        endBind();
    }
    void bindAddressFreq(String addr, int count, int cluster)
    {
        sqlite3_prepare(db, sql_insert_into[3], -1, &sql_insert_addr_fre, NULL);
        startBind();
        sqlite3_reset(sql_insert_addr_fre);
        sqlite3_bind_text(sql_insert_addr_fre, 1, addr.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(sql_insert_addr_fre,2,count);
        sqlite3_bind_int(sql_insert_addr_fre,3,cluster);
        int res = sqlite3_step(sql_insert_addr_fre);
        LOG_ASSERT_ERROR(res==SQLITE_DONE, "ERROR exexuting SQL statement %s", sqlite3_errmsg(db));
        endBind();
    }

    void bindAddressBasedFreq(int cluster, int size, String address)
    {
        sqlite3_prepare(db, sql_insert_into[4], -1, &sql_insert_addrbased_cluster, NULL);
        startBind();
        sqlite3_reset(sql_insert_addrbased_cluster);
        sqlite3_bind_int(sql_insert_addrbased_cluster,1,cluster);
        sqlite3_bind_int(sql_insert_addrbased_cluster,2,size);
        sqlite3_bind_text(sql_insert_addrbased_cluster,3,address.c_str(), -1, SQLITE_TRANSIENT);
        int res = sqlite3_step(sql_insert_addrbased_cluster);
        LOG_ASSERT_ERROR(res==SQLITE_DONE, "ERROR exexuting SQL statement %s", sqlite3_errmsg(db));
        endBind();
    }
};

class Count
{
    uint count;
    int cluster;
    public:
    Count(){this->count=1; this->cluster=-1;}
    void incrCount(){count++;}
    uint getCount(){return count;}
    void setCount(uint count){this->count+=count;}
    void setCluster(int cluster){this->cluster=cluster;}
    int getCluster(){return this->cluster;}
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

     //output files names
    String eipNodesInfo = "/Testeipnodes.out";
    String addNodesInfo = "/Testaddrnodes.out";
    String edgesInfo = "/Testedges.out";
    String eipFreqInfo = "/CSV_PCFrequency.csv"; //use eipFreq
    String addFreFileInfo = "/CSV_AddrFrequency.csv";
    String addrClusterFileInfo_csv = "/CSV_AddrCluster.csv";
    String addrClusterFileInfo = "/StatAddrCluster.csv";
    String pcBasedCluster = "/CSV_PcbasedCluster.csv";
    String pcBasedClusterStride = "/CSV_PcbasedClusterStride.csv";

    // addr to datainfo
    typedef std::map<String, DataInfo*> Add2Data;

    //* eip to access info
    std::map<String, Add2Data> table;

    //
    std::map<String, StrideCluster> strideClusterInfo;

    UInt32 last=0;

    UInt32 total=0,reeip=0,readdr=0;

    //* path(aka name of structure) to  requested address
    std::map<String, std::set<String>> path2haddrStorageInfo;

    // path(ake name of structure) to count of requested addresses
    std::map<String, UInt32> countByNameInfo;

    //* eip address cycle#
    std::vector<std::vector<String>> cycleInfo;
    String cycleInfoOutput="/CSV_CycleTrace.csv";
    String strideAddrOrderOutput="/StatEipBasedAddrOrder.out";

    String outputDirName;
    Storage *storage;
    StrideTable() {
        storage = new Storage();
    }

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
    StrideTable *strideTable = new StrideTable();
    bool memAccessType;// cache=0 dram=1
   
    public:
    CacheHelper(){}
    ~CacheHelper(){delete strideTable;}
    void writeOutput(){ strideTable->write();}
    void strideTableUpdate();
    void addRequest(Access* access);
    void addRequest(IntPtr eip, IntPtr addr, String objname, Cache* cache, UInt64 cycleCount, bool accessType, bool accessResult);
    std::stack<Access*> getRequestStack(){return this->request;}
    void setOutputDir(String outputDir){
        if(strideTable==NULL)
        {
            printf("nUll error");
            exit(0);
        }
         strideTable->setOutputDir(outputDir); }
};

};
#endif