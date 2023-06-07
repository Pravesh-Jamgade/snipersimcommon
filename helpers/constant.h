#ifndef CONST_H
#define CONST_H
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <algorithm>

enum class NEXT_LVL_CALL
{
    A=0,
    B,
    REC,
    INVALID=-1
};

enum class ACCESS_LEVEL
{
    ZERO=0,
    ONE,
    SHORT_BURST,
    LONG_BURST,
    REPEAT_BURST
};

enum class ARRAY_TYPE
{
    NONE=0,
    OFFSET_ARRAY,
    EDGE_ARRAY,
    PROPERTY_ARRAY
};

/*
+ consecutive accesses within last seen 10 accesses ==> short burst
+ consecutive accesses after last seen 10 accesses ==> large burst
TODO: Changet this constant value to see impact on judgement, evaluate by comparing how many accesses become short/large 
*/
const UInt64 TH_ACCESS_LEVEL = 10;
const UInt64 TH_ACCESS_LEVEL_UPPER = 50;

#endif