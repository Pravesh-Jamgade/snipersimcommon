#include "cache_helper.h"
#include <iomanip>


using namespace cache_helper;

void StrideTable::write()
{
    std::map<String, Add2Data>::iterator pit = table.begin();// iterator on eip add2data
    std::set<String>::iterator sit; // iterator on set of stride from StrideCluster
    std::map<String, StrideCluster>::iterator rit;
    std::vector<StrideOrder>::iterator strideOrderVecIt;

    for(;pit!=table.end();pit++)
    {
        Add2Data add2data = pit->second;
        Add2Data::iterator qit = add2data.begin(); // iterator on add2data
        
        for(;qit!=add2data.end(); qit++)
        {
                if(qit->second)
                {
                    DataInfo* dataInfo = qit->second;
                    _LOG_PRINT_CUSTOM(Log::Warning, "EIP=%s,ADDR=%s,LD=%ld,ST=%ld, T=%ld\n",
                    pit->first.c_str(), qit->first.c_str(), dataInfo->getCount(1), dataInfo->getCount(0), dataInfo->getCount(-1));
                }
        }
        _LOG_PRINT_CUSTOM(Log::Warning, "EIP=%s STRIDE=[", pit->first.c_str());
        rit = strideClusterInfo.find(pit->first);
        if(rit!=strideClusterInfo.end())
        {
            std::set<String>strideList = rit->second.getStrideList();
            sit = strideList.begin();
            for(;sit!=strideList.end();sit++)
            {
                // _LOG_PRINT(Log::Warning, "%ld, ",dataInfo->stride[i]);
                const char* p = &(*sit)[0];
                _LOG_PRINT_CUSTOM(Log::Warning, "%s, ", p);
            }
            // _LOG_PRINT(Log::Warning, "load=%ld store=%ld",dataInfo->typeCount[1], dataInfo->typeCount[0]);
        }
        _LOG_PRINT_CUSTOM(Log::Warning, "]\n");
    }

    _LOG_PRINT_CUSTOM(Log::Warning, "*********************************PATH-2-ADDRESSES*******************************\n");

    std::map<String, std::set<String>>::iterator pasit = path2haddrStorageInfo.begin();
    for(;pasit!=path2haddrStorageInfo.end();pasit++)
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

    printf("Total Access=%d\n", total);

    printf("Total Access by Access Type\n");
    std::map<String, UInt32>::iterator icn = countByNameInfo.begin();
    for(;icn!=countByNameInfo.end();icn++)
    {
        String name = icn->first;
        char* p=&name[0];
        printf("%s = %d\n", p, icn->second);
    }

    std::fstream outfile;
    cycleInfoOutput=outputDirName+cycleInfoOutput;
    outfile.open(cycleInfoOutput.c_str(), std::ios_base::out);
    if(outfile.is_open())
    {
        for(int i=0; i< cycleInfo.size(); i++)//cycleInfo is a vector
        {
           outfile<<cycleInfo[i]<<'\n';
        }
        outfile.close();
    }
    else std::cout<<"cycleLog.dat is not open"<<'\n';

    strideAddrOrderOutput=outputDirName+strideAddrOrderOutput;
    outfile.open(strideAddrOrderOutput.c_str(), std::ios_base::out);
    if(outfile.is_open())
    {
        rit = strideClusterInfo.begin();//eip to StrideCluster
                                        // strideCluster has info about: stride, StrideOrder
                                        // StrideOrder has info about: address pattern, address count(to optimize for analysis)
        for(;rit!=strideClusterInfo.end();rit++)
        {
            outfile<<rit->first<<" ORDER=[";
            std::vector<StrideOrder> strideOrder = rit->second.getStrideOrder();
            strideOrderVecIt = strideOrder.begin();
            for(;strideOrderVecIt!=strideOrder.end();strideOrderVecIt++)
            {
                outfile<<"("<<strideOrderVecIt->getAddress()<<","<<strideOrderVecIt->getAddrCount()<<"), ";
            }
            outfile<<"]\n";
        }
        outfile.close();
    }

    //for eip nodes
    String eipNodesInfo = "/eipnodes.csv";
    String addNodesInfo = "/addrnodes.csv";
    String edgesInfo = "/edges.csv";
    String eipFreqInfo = "/EipFrequency.csv"; //use eipFreq

    String eipNodesFilePath=outputDirName+eipNodesInfo;
    String addrNodesFilePath=outputDirName+addNodesInfo;
    String edgesFilePath=outputDirName+edgesInfo;
    String eipFreqFilePath=outputDirName+eipFreqInfo;

    std::fstream eipNodesFile, edgesFile, addNodesFile, eipFreqFile;

    eipNodesFile.open(eipNodesFilePath.c_str(), std::ios_base::out);
    addNodesFile.open(addrNodesFilePath.c_str(), std::ios_base::out);
    edgesFile.open(edgesFilePath.c_str(), std::ios_base::out);
    eipFreqFile.open(eipFreqFilePath.c_str(), std::ios_base::out);

    std::set<String> seenBefore;
    if(eipNodesFile.is_open() && edgesFile.is_open() && addNodesFile.is_open() && eipFreqFile.is_open())
    {
        eipNodesFile<<"EIP\n";
        addNodesFile<<"ADDRESS\n";
        edgesFile<<"EIP,ADDRESS\n";
        std::map<String, Add2Data>::iterator pit = table.begin();// iterator on eip add2data
        for(;pit!=table.end();pit++)
        {
            eipNodesFile<<pit->first<<'\n';
            Add2Data add2data = pit->second;
            Add2Data::iterator qit = add2data.begin();
            for(;qit!=add2data.end();qit++)
            {
                addNodesFile<<qit->first<<'\n';
                edgesFile<<pit->first<<','<<qit->first<<'\n';
            }
        }
        /*eipFrequency output*/
        std::map<String, UInt64>::iterator it = eipFreq.begin();
        eipFreqFile<<"EIP"<<","<<"Frquency\n";
        for(;it!=eipFreq.end();it++)
        {
            eipFreqFile<<it->first<<","<<it->second<<'\n';
        }
    }
    eipNodesFile.close(); edgesFile.close(); addNodesFile.close(); eipFreqFile.close();
}

void StrideTable::lookupAndUpdate(int access_type, IntPtr eip, IntPtr addr, String path, UInt64 cycleNumber, 
bool accessResult)
{   
    total++;

    String hindex, haddr, hcycle;
    hcycle = itostr(cycleNumber);

    IntPtr index = eip;

    std::stringstream ss;
    ss << std::hex << index; ss >> hindex; ss.clear();
    ss << std::hex << addr; ss >> haddr; ss.clear();
    
    String accessResStr;
    if(accessResult){accessResStr="H";}
    else {accessResStr = "M";}

    String accessTypeStr = access_type==1?"L":"S";
    String cycleInfoString = Misc::AppendWithSpace(accessTypeStr, accessResStr, path, hindex, haddr, hcycle);
    cycleInfo.push_back(cycleInfoString);

    // iteratros
    std::map<String, Add2Data>::iterator tableIt;
    Add2Data:: iterator addrIt;
    std::map<String, StrideCluster>::iterator strideIt;
    
    //store access path name to address
    path2haddrStorageInfo[path].insert(haddr);

    // find eip on table
    tableIt = table.find(hindex);
    // if eip found
    if(tableIt!=table.end())
    {
        reeip++;

        eipFreq[hindex]++;

        // get accessinfo for correspnding eip
        Add2Data *add2data = &tableIt->second;

        // find if addr is seen before, find corresponding data info
        addrIt = add2data->find(haddr);

        // if accessinfo found / addr seen before
        if(addrIt!=add2data->end())
        {
            readdr++;

            // get datainfo
            DataInfo* dataInfo = addrIt->second;
            // update datainfo
            dataInfo->incrTypeCount(access_type);
            dataInfo->incrTotalCount();            
        }
        else // eip seeen before but addr is new
        {
            add2data->insert( 
                    { haddr, new DataInfo(access_type) } 
            );
        }

        strideIt = strideClusterInfo.find(hindex);
        if(strideIt==strideClusterInfo.end())
        {
            std::cout<<"EIP is not here\n";
            exit(0);
        }
        strideIt->second.setStride(addr,haddr);
    }
    else
    {
        StrideCluster strideCluster(addr,haddr);
        strideClusterInfo[hindex]= strideCluster;
        table[hindex][haddr]=new DataInfo(access_type);
        eipFreq[hindex]=1;
    }

    std::map<String, UInt32>::iterator icn = countByNameInfo.find(path);
    if(icn!=countByNameInfo.end())
    {
        icn->second++;
    }
    else{
        countByNameInfo.insert({path, 1});
    }
}

void CacheHelper::addRequest(IntPtr eip, IntPtr addr, String objname, Cache* cache, UInt64 cycleCount, bool accessType, bool accessResult){
    IntPtr index = eip; // use lsb 20 bits for indexing, possible all instructions are in few blocks

    //todo: i can take some contant to generate these inforamtion regardless of where access is comming from
    if(cache==NULL)
    {
        // for dram access, i dont know how to mask it yet;
    }
    else
    {
        // for cache access, i am taking into account cache size, blocksize and calculating indexing information
    }
    IntPtr forStride = addr & 0xffffff;

    request.push(new Access(index, forStride, objname, cycleCount, accessType, accessResult));
}
void CacheHelper::addRequest(Access* access){request.push(access);}

void CacheHelper::strideTableUpdate()
{
    while(!getRequestStack().empty())
    {   
        Access* access = request.top(); 
        request.pop();
        char* p=&access->getObjectName()[0];
        strideTable.lookupAndUpdate(access->getAccessType(), access->getEip(), access->getAddr(), 
        access->getObjectName(), access->getCycleCount(), access->getAccessResult());
        std::cout<<access->getAccessType()<<" "<<access->getEip()<<" "<<access->getObjectName()<<" cycle="<<access->getCycleCount()<<std::endl;
        delete access;
    }
}

