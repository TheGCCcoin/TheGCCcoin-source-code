/*
Title: LiveLogging++

URL: https://github.com/d08ble/livelogging-cpp

MIT License

Copyright (c) 2014-2018 Web3 Crypto Wallet Team

 info@web3cryptowallet.com

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

// __LIVELOG_H [

#ifndef __LIVELOG_H
#define __LIVELOG_H

#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <iterator>
#include <time.h>
#include <unistd.h>

namespace acpul {

    // LiveLogBasic [

    class LiveLogBasic {
    protected:
        std::wofstream *_fs;
        std::wstring _path;
        std::vector<std::wstring> _stack;
//        bool _empty;

    public:
        LiveLogBasic(const char *filename)
        : _path(L"")
//        , _empty(false)
        , _fs(new std::wofstream(filename))
        {}

        ~LiveLogBasic()
        {}

        void begin(std::wstring path)
        {
            _stack.push_back(_path);
            _path = path;
            *_fs << L"# " << _path << L"[\n";
//            _empty = true;
        }

        void end()
        {
//            if (!_empty) {
//                _fs << L"# " << _path << L"]\n";
//            }
//            _empty = true;
            *_fs << L"# " << _path << L"]\n";
            _path = L"";

            if (_stack.size() > 0) {
                _path = _stack.back();
                _stack.pop_back();
            }
        }

        void put(std::wstring s)
        {
//            if (_empty) {
//                _fs << L"# " << _path << L"[\n";
//                _empty = false;
//            }

            int n = wcslen(s.c_str());
//            printf("%i\n", n);
//            printf("%i %S\n", n, s.c_str());

            *_fs << s << L"\n";
            _fs->flush();
        }

        void flush()
        {
            _fs->flush();
        }
    };

    // LiveLogBasic ]
    // LCNode [

    class LCNode {
        std::wstring _name;
        typedef std::map<std::wstring, LCNode *> mnodes;
        mnodes _items;
        std::vector<LCNode *> _itemsIndex;
        std::wstringstream _ss;

    public:
        LCNode(std::wstring name)
        : _name(name)
        {}

        ~LCNode()
        {
            clear();
        }

        const std::wstring &name()
        {
            return _name;
        }

        void clearText()
        {
            _ss.str(L"");
            _ss.clear();
        }

        const std::wstring text()
        {
            return _ss.str();
        }

        const std::map<std::wstring, LCNode *> &items()
        {
            return _items;
        }

        const std::vector<LCNode *> &itemsIndex()
        {
            return _itemsIndex;
        }

        void clear()
        {
            for (mnodes::iterator it = _items.begin(); it != _items.end(); it++) {
                delete it->second;
            }
            _items.clear();
            _itemsIndex.clear();
        }

        void log(std::vector<std::wstring> &path, std::wstring s, bool replace)
        {
            LCNode *node = allocNode(path, 0);
            if (!node)
                return;

            if (replace) {
                node->clearText();
            }

            node->put(s);
        }

        void put(std::wstring s)
        {
            _ss << s;
        }

        LCNode *allocNode(std::vector<std::wstring> &path, int i)
        {
            if (path.size() <= i)
                return this;

            std::wstring name = path[i];
            mnodes::iterator it = _items.find(name);
            LCNode *node;

            if (it == _items.end()) {
                node = _items[name] = new LCNode(name);
                _itemsIndex.push_back(node);
            }
            else
                node = it->second;

            return node->allocNode(path, i + 1);
        }

        void deleteNode(std::vector<std::wstring> path)
        {}
    };

    // LCNode ]
    // LiveLog [

    class LiveLog : public LiveLogBasic {
        LCNode _tree;
        std::string _filename;
        bool _modified;

        time_t _timeFlushPrev;

        int _flushTimeout;
        double _flushTimeDelta;

    public:
        LiveLog(const char *filename)
        : LiveLogBasic(filename)
        , _tree(L"LiveLog++")
        , _filename(filename)
        , _modified(true)
        , _flushTimeout(0)
        {}

        ~LiveLog()
        {}

        void setFilename(std::string filename)
        {
            _filename = filename;
        }

        std::string filename()
        {
            return _filename;
        }

        void setFlushTimeout(int flushTimeout)
        {
            _flushTimeout = flushTimeout;
        }

        void log(std::wstring path, std::wstring s, bool replace)
        {
            _modified = true;

            std::vector<std::wstring> vstrings;

            std::wstringstream ss(path);
            std::wstring item;
            while (std::getline(ss, item, L'/')) {
                vstrings.push_back(item);
                //*(result++) = item;
            }

            _tree.log(vstrings, s, replace);
        }

        void log(std::wstring path, std::wstring s)
        {
            log(path, s, false);
        }

        void replace(std::wstring path, std::wstring s)
        {
            log(path, s, true);
        }

        void erase(std::wstring path)
        {
            _modified = true;
        }

        void flush(bool force = false)
        {
            if (!_modified)
                return;

            // check time diff [

            if (!force && _flushTimeout > 0) {
                time_t timeEnd;
                time(&timeEnd);
                double timeDiff;
                timeDiff = difftime(timeEnd, _timeFlushPrev);
                _timeFlushPrev = timeEnd;
                _flushTimeDelta += timeDiff;

                if (_flushTimeDelta < _flushTimeout)
                    return;

                _flushTimeDelta = 0;
            }

            // check time diff ]

            if (_filename == "")
                return;

            // clear hack
            _fs->close();

            delete _fs;
            _fs = new std::wofstream(_filename.c_str());

            flushNode(&_tree);

            _fs->flush();

            _modified = false;
        }

        void flushNode(LCNode *node)
        {
            if (!node)
                return;

            begin(node->name());

            put(node->text());

            //for (std::vector<LCNode *>::iterator it = node->itemsIndex().begin(); it != node->itemsIndex().end(); it++) {
//            flushNode(*it);
            for (int i = 0; i < node->itemsIndex().size(); i++) {
                flushNode(node->itemsIndex()[i]);
            }

            end();
        }
    };

    // LiveLog ]
}

#endif

// __LIVELOG_H ]
