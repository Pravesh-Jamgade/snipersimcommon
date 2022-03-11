#ifndef CACHE_HELPER
#define CACHE_HELPER

#include<map>
#include "fixed_types.h"
// #include "cache.h"
        #include <stdio.h>      /* printf */
        #include <unistd.h>
        #include "log.h"
        #include<sstream>

        #include <fstream>
        #include <iostream>
        #include <unistd.h>
        #include<stack>
        #include<vector>
        #include<list>
        #include<memory>
        #include<sqlite3.h>

namespace cache_helper
{   

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
    UInt32 count;
    int cluster;
    public:
    Count(int zero){this->count=0, this->cluster=-1;}
    Count(){this->count=1; this->cluster=-1;}
    void incrCount(){count++;}
    void incrCount(int x){count=count+x;}
    UInt32 getCount(){return count;}
    void setCluster(int y){cluster=y;}
    int getCluster(){return cluster;}
};

/*Addresses in-order and their count if they are consecutive: Part of StrideCluster, cluster by PC or EIP*/
class AddressFrequencyAndOrder
{
    String address;
    UInt32 count;
    public:
    AddressFrequencyAndOrder(String addr):address(addr), count(1){} 

    bool operator==(AddressFrequencyAndOrder pre) {return address==pre.address;}
    void incrCount() {count++;}
    String getAddress(){return address;}
    UInt32 getAddrCount(){return count;}
};

/*List of Stride seen by particular PC or EIP based on addresses it has accesed, 
Order of addreses (if address is acceses consecutive then its count accounted by AddressFrequencyAndOrder class) as well */
class StrideCluster
{
    std::map<String,Count> strideList;//tracks only stride
    std::list<AddressFrequencyAndOrder> strideOrderList;//tracks addresses responsible for stride
    IntPtr lastSeenAddr=0;//tracks last seen address, to calculate stride for next access
    public:
    StrideCluster(){}
    StrideCluster(IntPtr initAddr, String haddr){ 
        AddressFrequencyAndOrder newAddressFrequencyAndOrderElement(haddr); // create new strideOrder and track it, in hex form
        strideOrderList.emplace_back(newAddressFrequencyAndOrderElement);//incoming address order
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
        std::map<String,Count>::iterator it=strideList.find(diffStr);
        if(it!=strideList.end()){
            it->second.incrCount();
        }
        else strideList[diffStr]=Count(); 

        AddressFrequencyAndOrder newstrideOrderElement(haddr);
        if(newstrideOrderElement==strideOrderList.back()){
            strideOrderList.back().incrCount();
            return;
        }//if prev strideorder == newstrideorder skip, otherwise track
        strideOrderList.emplace_back(newstrideOrderElement);
    }
    std::map<String, Count> getStrideList(){return strideList;}
    std::list<AddressFrequencyAndOrder> getAddressFrequencyAndOrder(){return strideOrderList;}
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
    void incrTotalCount(){this->total_access++;}
    void incrTypeCount(int accessType){this->typeCount[accessType]+=1;}
    UInt32 getCount(int type){ if(type<0){return this->total_access;}else{return this->typeCount[type];} }
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
    String cycleInfoOutput="/CSV_CycleTrace.csv";
    String strideAddrOrderOutput="/StatEipBasedAddrOrder.out";
    String accessCountOfCluster="/CSV_AccessCountOfCluster.csv";

    // addr to datainfo
    typedef std::map<String, DataInfo*> Add2Data;

    //* eip to access info
    std::map<String, Add2Data> table;

    //
    std::map<String, StrideCluster*> strideClusterInfo;

    UInt32 last=0;

    UInt32 total=0,reeip=0,readdr=0;

    //* path(aka name of structure) to  requested address
    std::map<String, std::set<String>> path2haddrStorageInfo;

    // path(ake name of structure) to count of requested addresses
    std::map<String, UInt32> countByNameInfo;

    //* eip address cycle#
    std::list<std::vector<String>> cycleInfo;
    
    // collect unique eip and addr
    std::map<String, Count> uniqueEIP, uniqueAddr;
    std::list<std::pair<String, String>> uniqueEdgePair;


    String outputDirName;

    std::fstream onlineoutput;

    StrideTable() {
        cycleInfoOutput=outputDirName+cycleInfoOutput;
        std::cout<<"open file to write:"<<cycleInfoOutput<<'\n';
        // onlineoutput.open(cycleInfoOutput.c_str(), std::ofstream::out | std::ofstream::app);
    }
    ~StrideTable() {
        onlineoutput.close();
    }

    void lookupAndUpdate(int access_type, IntPtr eip, IntPtr addr, String path, UInt64 cycleNumber, bool accessResult, int core);
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
    int core;
    public:
    Access(IntPtr eip, IntPtr addr, String objectName, UInt64 cycleCount, bool accessType, bool accessResult, int core){
        this->eip=eip; this->addr=addr; this->objectName=objectName; this->instAccessType=accessType;
        this->cycleCount=cycleCount; this->accessResult=accessResult; this->core=core;
    }

    String& getObjectName(){return this->objectName;}
    IntPtr getEip(){return this->eip;}
    bool getAccessType(){return this->instAccessType;}
    IntPtr getAddr(){return this->addr;}
    UInt64 getCycleCount(){return this->cycleCount;}
    bool getAccessResult(){return this->accessResult;}
    int getCoreId(){return this->core;}
};

class CacheHelper
{
    std::list<Access*> request;
    StrideTable* strideTable;
    
    public:
    CacheHelper(){
        strideTable = new StrideTable();
    }
    ~CacheHelper(){
        strideTable->write();
        delete strideTable;
    }

    void addRequest(IntPtr eip, IntPtr addr, String objname, UInt64 cycleCount, int core, bool accessType, bool accessResult);
    std::list<Access*> getRequestStack(){return request;}
    int getReqStackSize(){return request.size();}
    void setOutputDir(String outputDir){
        strideTable->setOutputDir(outputDir); 
    }
};

};
#endif