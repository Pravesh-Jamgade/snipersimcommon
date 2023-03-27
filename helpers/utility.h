#ifndef FILESTREAM_H
#define FILESTREAM_H

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <algorithm>

using namespace std;
class FILESTREAM{
    public:
    static fstream get_file_stream(string name){
        fstream newfs;
        newfs.open(name.c_str(), std::fstream::app | std::fstream::in);
        return newfs;
    }
    private:
};
#endif