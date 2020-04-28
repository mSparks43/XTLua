//
//  xpmtdatatypes.h
//  xTLua
//
//  Created by Mark Parker on 04/19/2020
//
//	Copyright 2020
//	This source code is licensed under the MIT open source license.
//	See LICENSE.txt for the full terms of the license.

#include <vector>
class XTLuaDouble
{
    public:
    std::vector<double> values;
    void * ref;
    int size;
    int end;//maybe we dont parse every value
    int start;//maybe we dont parse every value
    bool isArray;//more than one value
    bool get;//do we get this data
    bool set;//do we set this data

};
class XTLuaArrayFloat{
    public:
    float value;
    void * ref;
    int type;
    int index;
    bool get;//do we get this data
    bool set;//do we set this data
};
class XTLuaFloat
{
    public:
    
    std::vector<XTLuaArrayFloat> values;
    void * ref;
    bool get;//do we get this data
    bool set;//do we set this data

};
class XTLuaInteger
{
    public:
    std::vector<int> values;
    void * ref;
    int size;
    int end;//maybe we dont parse every value
    int start;//maybe we dont parse every value
    bool isArray;//more than one value
    bool get;//do we get this data
    bool set;//do we set this data

};
class XTLuaChars
{
    public:
    std::vector<char> values;
    void * ref;
    //int size;
    //int end;//maybe we dont parse every value
    //int start;//maybe we dont parse every value
    bool get;//do we get this data
    bool set;//do we set this data
};

