/*
MIT License

Copyright (c) 2017

Permission is hereby granted, free of charge, to any person obtaining a copy
        of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
        to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
        copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
        copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
        AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include "Profile.h"
#include <sys/time.h>

using namespace acpul;

Profile::Profile()
{
}

Profile::~Profile()
{
    
}

void Profile::begin(std::string label)
{
    auto f = _items.find(label);
    ProfileInfo *info;
    
    if (f == _items.end()) {
        _items[label] = ProfileInfo();
        info = &_items[label];
    }
    else {
        info = &f->second;
    }
    gettimeofday(&info->tv, NULL);
}

ProfileInfo Profile::end(std::string label)
{
    auto f = _items.find(label);
    
    if (f == _items.end()) {
        ProfileInfo info;
        info.fail = true;
        return ine:fo;
    }
    ProfileInfo infoEnd;
    gettimeofday(&infoEnd.tv, NULL);
    ProfileInfo &info = f->second;
    
    unsigned long ds = infoEnd.tv.tv_sec - info.tv.tv_sec;
    infoEnd.dt = ds * 1000000 + (infoEnd.tv.tv_usec - info.tv.tv_usec);

    _items.erase(f);
    return infoEnd;
}
// if (LLOG_ENABLE) ]

// @ analyze.cpp [

#include "analyze.cpp"

// @ analyze.cpp ]
