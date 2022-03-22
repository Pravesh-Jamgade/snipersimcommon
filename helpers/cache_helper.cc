#include "cache_helper.h"
#include <iomanip>


using namespace cache_helper;

void StrideTable::write()
{
    std::fstream pcBasedClusterFile, pcBasedClusterStrideFile;
    String pcBasedClusterFilePath=outputDirName+pcBasedCluster;
    String pcBasedClusterStrideFilePath=outputDirName+pcBasedClusterStride;
    pcBasedClusterFile.open(pcBasedClusterFilePath.c_str(), std::ofstream::out | std::ofstream::app);
    pcBasedClusterStrideFile.open(pcBasedClusterStrideFilePath.c_str(), std::ofstream::out | std::ofstream::app);
    pcBasedClusterFile<<"pc,address,count\n";
    pcBasedClusterStrideFile<<"pc,stride,count\n";
   
    std::map<String, Add2Data>::iterator pit = table.begin();// iterator on eip add2data
    std::unordered_map<String,Count>::iterator sit; // iterator on set of stride from StrideCluster
    std::map<String, StrideCluster*>::iterator rit;
    std::list<AddressFrequencyAndOrder>::iterator strideOrderVecIt;

    for(;pit!=table.end();pit++)//pc or eip
    {
        Add2Data add2data = pit->second;
        Add2Data::iterator qit = add2data.begin(); // iterator on add2data
        
        for(;qit!=add2data.end(); qit++)// addr
        {
            uniqueEdgePair.push_back({pit->first, qit->first});

            if(qit->second)
            {
                DataInfo* dataInfo = qit->second;
                // pc or eip, addr, count
                pcBasedClusterFile<<pit->first.c_str()<<","<<qit->first.c_str()<<","<<dataInfo->getCount(-1)<<'\n';
            }
        }
        rit = strideClusterInfo.find(pit->first);// for pc or eip find stridecluster
        if(rit!=strideClusterInfo.end())
        {
            // stride and count
            std::unordered_map<String,Count>strideList = rit->second->getStrideList(); // get strideList for pc or eip
            sit = strideList.begin();
            for(;sit!=strideList.end();sit++)
            {
                // pc or eip, stride, count
                pcBasedClusterStrideFile<<pit->first<<","<<sit->first<<","<<sit->second.getCount()<<'\n';
            }
        }
    }

    pcBasedClusterFile.close();
    pcBasedClusterStrideFile.close();

    printf("Total Access=%d\n", total);
    printf("Total Access by Access Type\n");

    std::map<String, UInt32>::iterator icn = countByNameInfo.begin();
    for(;icn!=countByNameInfo.end();icn++)
    {
        String name = icn->first;
        char* p=&name[0];
        printf("%s = %d\n", p, icn->second);
    }

    /*output, stride order followed on particular EIP: EIP, ORDER[address (access_count),...]*/
    strideAddrOrderOutput=outputDirName+strideAddrOrderOutput;
    std::fstream outfile;
    outfile.open(strideAddrOrderOutput.c_str(), std::ofstream::out | std::ofstream::app);
    if(outfile.is_open())
    {
        rit = strideClusterInfo.begin();//eip to StrideCluster std::map<String,StrideCluster>
                                        // strideCluster has info about: stride, AddressFrequencyAndOrder
                                        // AddressFrequencyAndOrder has info about: address pattern, address count(to optimize for analysis)
        for(;rit!=strideClusterInfo.end();rit++)
        {
            outfile<<rit->first<<" ORDER=[";
            std::list<AddressFrequencyAndOrder> strideOrder = rit->second->getAddressFrequencyAndOrder();
            strideOrderVecIt = strideOrder.begin();

            // iterating on objcet of AddressFrequencyAndOrder vector
            for(;strideOrderVecIt!=strideOrder.end();strideOrderVecIt++)
            {
                outfile<<"("<<strideOrderVecIt->getAddress()<<","<<strideOrderVecIt->getAddrCount()<<"), ";
            }

            outfile<<"]\n";
        }
        outfile.close();
    }

    // output file path
    String eipNodesFilePath=outputDirName+eipNodesInfo;
    String addrNodesFilePath=outputDirName+addNodesInfo;
    String edgesFilePath=outputDirName+edgesInfo;
    String eipFreqFilePath=outputDirName+eipFreqInfo;
    String addrFreqFilePath=outputDirName+addFreFileInfo;
    String addrClusterFilePath=outputDirName+addrClusterFileInfo;
    String addrClusterFile_csvPath=outputDirName+addrClusterFileInfo_csv;
    String accessCountOfCluster_csvPath=outputDirName+accessCountOfCluster;

    // filestream objects
    std::fstream eipNodesFile, edgesFile, addrNodesFile, eipFreqFile, 
    addrFreFile, addrClusterFile, addrClusterFile_csv, accessCountClusterFile;
    
    // open files
    eipNodesFile.open(eipNodesFilePath.c_str(), std::ofstream::out | std::ofstream::app);
    addrNodesFile.open(addrNodesFilePath.c_str(), std::ofstream::out | std::ofstream::app);
    edgesFile.open(edgesFilePath.c_str(), std::ofstream::out | std::ofstream::app);
    eipFreqFile.open(eipFreqFilePath.c_str(), std::ofstream::out | std::ofstream::app);
    addrFreFile.open(addrFreqFilePath.c_str(), std::ofstream::out | std::ofstream::app);
    addrClusterFile.open(addrClusterFilePath.c_str(), std::ofstream::out | std::ofstream::app);
    addrClusterFile_csv.open(addrClusterFile_csvPath.c_str(), std::ofstream::out | std::ofstream::app);
    accessCountClusterFile.open(accessCountOfCluster_csvPath.c_str(),std::ofstream::out | std::ofstream::app);

    // eip frequency
    std::map<String, Count>::iterator eipFreIt = uniqueEIP.begin();
    eipFreqFile<<"pc,frequency\n";
    for(;eipFreIt!=uniqueEIP.end(); eipFreIt++)
    {
        eipFreqFile<<eipFreIt->first<<","<<eipFreIt->second.getCount()<<"\n";
        //eip nodes
        eipNodesFile<<eipFreIt->first<<"\n";
    }
    eipNodesFile.close();
    eipFreqFile.close();

    // address frequency
    std::map<String, Count>::iterator addrFreIt;

    // addr and cluster
    bool allow;
    int64_t prev, diff;
    std::vector<String> bag;
    std::vector<std::vector<String>> clusters;
    std::vector<std::pair<String, Count>> addrCountVec;
    addrCountVec.assign(uniqueAddr.begin(), uniqueAddr.end());
    addrCountVec.push_back({"0x0", Count()});
    prev=-1;

    bag.push_back(addrCountVec[0].first);

    for(int i=0; i<= addrCountVec.size()-2 && addrCountVec.size()>3; i++)
    {
        String addr = addrCountVec[i].first;
        String next_addr = addrCountVec[i+1].first;
        int64_t addInt, next_addInt;
        std::stringstream ss;
        ss <<std::hex<< addr; ss>>addInt; ss.clear();
        ss <<std::hex<<next_addr; ss>>next_addInt; ss.clear();
        
        ss.clear();

        allow=true;

        diff = next_addInt - addInt;
        if(diff<0)
            diff=-1*diff;
        // std::cout<<addr<<"="<<addInt<<","<<next_addr<<"="<<next_addInt<<","<<prev<<","<<diff<<'\n';

        if(prev==-1){
            prev=diff;
            continue;
        }
        if(diff!=prev)
        {
            if(diff>prev)
            {
                bag.push_back(addr);
                allow=false;
            }
            else{}
            if(bag.size()!=0)
            {
                clusters.push_back(bag);
            }
            bag.erase(bag.begin(),bag.end());
            prev=diff;
        }

        if(allow)
            bag.push_back(addr);
    }

    std::map<String, Count>::iterator vvcit;
    for(int i=0; i< clusters.size(); i++)
    {
        for(int j=0; j< clusters[i].size(); j++)
        {
            vvcit=uniqueAddr.find(clusters[i][j]);
            if(vvcit!=uniqueAddr.end())
            {
                vvcit->second.setCluster(i);
            }
        }
    }

    accessCountClusterFile<<"clusterNo, accessFrequency\n";
    addrClusterFile_csv<<"clusterNo,address\n";
    for(int i=0; i< clusters.size(); i++)
    {
        int totalAddressAccessForCluster_i=0;
        // cluster number
        addrClusterFile<<i<<" [";
        for(int j=0; j< clusters[i].size(); j++)
        {
            vvcit=uniqueAddr.find(clusters[i][j]);
            if(vvcit!=uniqueAddr.end())
            {
                vvcit->second.setCluster(i);
                totalAddressAccessForCluster_i=totalAddressAccessForCluster_i+vvcit->second.getCount();
            }
            // address belonging to cluster
            addrClusterFile<<clusters[i][j]<<"  ";

            // clusterNo, addresss
            addrClusterFile_csv<<i<<","<<clusters[i][j]<<'\n';
        }
        //cluster number, total access done by pc on addresses which are part of cluster
        accessCountClusterFile<<i<<","<<totalAddressAccessForCluster_i<<'\n';
        addrClusterFile<<"]\n";
    }
    addrClusterFile.close();
    addrClusterFile_csv.close();

    addrFreFile<<"address,frequency,clusterNo,clusterSize\n";
    for(addrFreIt = uniqueAddr.begin(); addrFreIt!=uniqueAddr.end(); addrFreIt++)
    {
        int clusterNumber = addrFreIt->second.getCluster();
        int clusterSize = clusters[clusterNumber].size();
        addrFreFile<<addrFreIt->first<<","<<addrFreIt->second.getCount()<<","<<clusterNumber<<","<<clusterSize<<"\n";
        //addr nodes
        addrNodesFile<<addrFreIt->first<<"\n";
    }
    addrNodesFile.close();
    addrFreFile.close();
    accessCountClusterFile.close();

    // edge
    std::list<std::pair<String, String>>::iterator edgeIt = uniqueEdgePair.begin();
    edgesFile<<"pc,address\n";
    for(; edgeIt!=uniqueEdgePair.end(); edgeIt++)
    {
        edgesFile<<edgeIt->first<<","<<edgeIt->second<<"\n";
    }
    edgesFile.close(); 

}

void StrideTable::lookupAndUpdate(int access_type, IntPtr eip, IntPtr addr, String path, UInt64 cycleNumber, 
bool accessResult, int core)
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
    // String cycleInfoString = Misc::AppendWithSpace(accessTypeStr, accessResStr, path, hindex, haddr, hcycle);
    // cycleInfo.emplace_back(std::vector<String>{accessTypeStr,accessResStr,path,hindex,haddr,hcycle,itostr(core)});

    //onlineoutput<<accessTypeStr<<","<<accessResStr<<","<<path<<","<<hindex<<","<<haddr<<","<<hcycle<<","<<itostr(core)<<std::endl;

    _LOG_PRINT_CUSTOM(Log::Warning, "%s,%s,%s,%s,%s,%s,%s\n", accessTypeStr.c_str(),
    accessResStr.c_str(),path.c_str(),hindex.c_str(),haddr.c_str(),hcycle.c_str(),itostr(core).c_str());
    // iteratros
    std::map<String, Add2Data>::iterator tableIt;
    Add2Data:: iterator addrIt;
    std::map<String, StrideCluster*>::iterator strideIt;
    
    //store access path name to address
    path2haddrStorageInfo[path].insert(haddr);

    // find eip on table
    tableIt = table.find(hindex);
    // if eip found
    if(tableIt!=table.end())
    {
        // get accessinfo for correspnding eip
        Add2Data *add2data = &tableIt->second;

        // find if addr is seen before, find corresponding data info
        addrIt = add2data->find(haddr);

        // if accessinfo found / addr seen before
        if(addrIt!=add2data->end())
        {
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
    }
    else table[hindex][haddr]=new DataInfo(access_type);

    strideIt = strideClusterInfo.find(hindex);
    if(strideIt==strideClusterInfo.end()) strideClusterInfo.insert({hindex,new StrideCluster(addr,haddr)});
    else strideIt->second->setStride(addr,haddr);

    std::map<String, Count>::iterator uniqIt;

    uniqIt=uniqueAddr.find(haddr);
    if(uniqIt!=uniqueAddr.end()) uniqIt->second.incrCount();
    else uniqueAddr.insert({haddr, Count()});

    uniqIt=uniqueEIP.end();
    uniqIt=uniqueEIP.find(hindex);
    if(uniqIt!=uniqueEIP.end()) uniqIt->second.incrCount();
    else uniqueEIP.insert({hindex, Count()});

    std::map<String, UInt32>::iterator icn = countByNameInfo.find(path);
    if(icn!=countByNameInfo.end()) icn->second++;
    else countByNameInfo.insert({path, 1});
}

void CacheHelper::addRequest(IntPtr eip, IntPtr addr, String objname, UInt64 cycleCount, int core, bool accessType, bool accessResult)
{
    IntPtr index = eip; // use lsb 20 bits for indexing, possible all instructions are in few blocks
    IntPtr forStride = addr >> 6;
    strideTable->lookupAndUpdate(
        accessType,
        eip,
        forStride,
        objname,
        cycleCount,
        accessResult,
        core
        );
}
