#include "cache_helper.h"
#include <iomanip>

using namespace cache_helper;

void StrideTable::write()
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

    printf("Total Access=%d\n", total);

    printf("Total Access by Access Type\n");
    std::map<String, UInt32>::iterator icn = countByName.begin();
    for(;icn!=countByName.end();icn++)
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
        for(int i=0; i< cycleInfo.size(); i++)
        {
           outfile<<cycleInfo[i]<<std::endl;
        }
        outfile.close();
    }
    else std::cout<<"cycleLog.dat is not open"<<'\n';
    
    
}

void StrideTable::lookupAndUpdate(int access_type, IntPtr eip, IntPtr addr, String path, UInt64 cycleNumber)
{   
    total++;
    // for stride calculation
    UInt32 tmp = addr;
    UInt32 diff = abs((long)tmp - last);

    // for indexing,
    IntPtr index = eip;

    std::stringstream ss;
    String hindex, haddr, hcycle;
    ss << std::hex << index; ss >> hindex; ss.clear();
    ss << std::hex << addr; ss >> haddr; ss.clear();
    ss << std::hex << cycleNumber; ss >> hcycle; ss.clear();
    
    String cycleInfoString = Misc::AppendWithSpace(hindex, haddr, hcycle);
    cycleInfo.push_back(cycleInfoString);

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
                    { haddr, new DataInfo(access_type, diff) } 
            );
        }
    }
    else
    {
        table[hindex][haddr]=new DataInfo(access_type, diff);
    }

    last = tmp;

    std::map<String, UInt32>::iterator icn = countByName.find(path);
    if(icn!=countByName.end())
    {
        icn->second++;
    }
    else{
        countByName.insert({path, 1});
    }
}

void CacheHelper::addRequest(IntPtr eip, IntPtr addr, String objname, Cache* cache, UInt64 cycleCount, bool accessType, bool accessResult){
    IntPtr index = eip & 0xfffff; // use lsb 20 bits for indexing, possible all instructions are in few blocks
    IntPtr addrForStride = addr;

    //todo: i can take some contant to generate these inforamtion regardless of where access is comming from
    if(cache==NULL)
    {
        // for dram access, i dont know how to mask it yet;
    }
    else
    {
        // for cache access, i am taking into account cache size, blocksize and calculating indexing information
    }
    IntPtr forStride = addr & 0xfffff;

    request.push(new Access(index, forStride, objname, cycleCount, accessType));
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
        access->getObjectName(), access->getCycleCount());
        std::cout<<access->getAccessType()<<" "<<access->getEip()<<" "<<access->getObjectName()<<" cycle="<<access->getCycleCount()<<std::endl;

        delete access;
    }
}

