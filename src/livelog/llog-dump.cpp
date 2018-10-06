/*
Title: Dash v12.3 / Bitcoin Core / Blockchain LiveLogging++ dump

MIT License

Copyright (c) 2018 Web3 Crypto Wallet Team

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
// @ [

#include <sys/stat.h>
#include <pthread.h>

#include "base58.h"
#include "script.h"

#include "wallet.h"
#include "walletdb.h"
#include "bitcoinrpc.h"
#include "init.h"
#include "base58.h"
#include "stealthaddress.h"

#include "livelog.h"
#include "llog-dump.h"

#include "../main.h"

using namespace std;
using namespace acpul;
using namespace json_spirit;

// @ ]
// vars [

const wchar_t *liveLoggingBannerShort = L"Dash v12.3 / Bitcoin Core / Blockchain LiveLogging++ dump\n"
        "MIT License\n"
        "Copyright (c) 2014-2018 Web3 Crypto Wallet Team\n"
        " info@web3cryptowallet.com\n";

LiveLog *llog = NULL;

pthread_mutex_t _llogMutex = PTHREAD_MUTEX_INITIALIZER;

// vars ]
// LiveLog c [

void livelogFlushThreadStart();
void livelogFlushThreadStop();
void llogFlush(bool force = false);

pthread_t _threadLiveLogFlush;
int _threadLiveLogFlushId;

bool _threadLiveLogRunning = false;
int livelogFlushTimeout = 10; // 10s

static void *_llFlushThread(void *argument)
{
    time_t timePrev;
    time(&timePrev);

    time_t timeStart;
    time(&timeStart);

    while (_threadLiveLogRunning) {
        llogFlush(true);
        sleep(livelogFlushTimeout);

        time_t timeEnd;
        time(&timeEnd);
        double timeDiff;
        timeDiff = difftime(timeEnd, timePrev);
        timePrev = timeEnd;

//        std::stringstream ss;
//        ss << timeEnd;
//        llogLog(L"LLOG/FlushThread", L"Last update", ss.str(), true);

        char buffer[1024];

        double uptimeDiff = difftime(timeEnd, timeStart);
        std::tm *ptm = std::localtime(&timeStart);

        // Format: Mo, 15.06.2009 20:20:00
        std::strftime(buffer, 1024, "%a, %d.%m.%Y %H:%M:%S", ptm);
        llogLog(L"LLOG/FlushThread", L"Start time", buffer, true);
        llogLog(L"LLOG/FlushThread", L"uptime", uptimeDiff);

        ptm = std::localtime(&timeEnd);
        std::strftime(buffer, 1024, "%a, %d.%m.%Y %H:%M:%S", ptm);

        llogLog(L"LLOG/FlushThread", L"Last update", buffer);
        llogLog(L"LLOG/FlushThread", L"diff", timeDiff);
    }
}

void livelogFlushThreadStart()
{
    pthread_t thread1;

    _threadLiveLogRunning = true;

    _threadLiveLogFlushId = pthread_create(&_threadLiveLogFlush, NULL, _llFlushThread, (void *) "LiveLog Flush Thread");

    llogLog(L"LLOG/FlushThread", L"FlushThread Started");
    llogFlush(true);
}

void livelogFlushThreadStop()
{
    _threadLiveLogRunning = false;

    pthread_join(_threadLiveLogFlush, NULL);

    llogLog(L"LLOG/FlushThread", L"FlushThread Stopped");
    llogFlush(true);
}

void llogFlush(bool force)
{
    pthread_mutex_lock(&_llogMutex);
    llog->flush(force);
    pthread_mutex_unlock(&_llogMutex);
}

// LiveLog c ]
// utils [

void lazyinit();

bool isFileExists(const std::string& name);
void moveLog(const char *name);

bool isFileExists(const std::string& name)
{
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

void moveLog(const char *name, const char *toname)
{
    if (isFileExists(name)) {
        for (int i = 0; ; i++) {
            std::ostringstream ss;
            ss << toname << "." << i << ".sh";
            std::string newName(ss.str());
            if (!isFileExists(newName)) {
                rename(name, newName.c_str());
                break;
            }
        }
    }
}

// LiveLog setup [

#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp> // for startswith() and endswith()
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options/detail/config_file.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/thread.hpp>
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/conf.h>
#include "util.h"

void llogSetup(std::string filename)
{
    lazyinit();

//        boost::filesystem::path defaultDataDir = GetDefaultDataDir();
//        boost::filesystem::path dataDir = GetDataDir();

    if (filename == "") {
        std::string livelogDefaultDir = "";
/*        std::string livelogDir = GetArg("-livelog", livelogDefaultDir);

        // datadir is not set - skip
        if (livelogDir == "")
            return;

        filename = livelogDir + "/livelog-log.sh";*/

        filename = GetArg("-livelog", livelogDefaultDir);
        if (filename == "")
            return;
    }


    llog->setFilename(filename);
}

// LiveLog setup ]

void lazyinit()
{
    if (!llog) {
        llog = new LiveLog("");

        livelogFlushTimeout = 10;
        llog->setFlushTimeout(10);

        llogLog(L"LLOG/About", liveLoggingBannerShort);
        llogLog(L"LLOG/DUMP", L"Started");

        livelogFlushThreadStart();

        llogFlush(true);
    }

//    if (llog->filename() != livelogDir) {
//        const char *filename = "/home/user/src5/gcc/log/thegcc.sh";
//        const char *toname = "/home/user/src5/gcc/log/0/thegcc";
//        moveLog(filename, toname);
//        llog->setFilename(livelogDir)
//    }
}

/*
void dumpMessage(std::wstring msg)
{
    lazyinit();
    llog->put(msg);
}
*/

// utils ]
// LiveLogBasic - old [

void llogBegin(std::wstring path)
{
    lazyinit();

    pthread_mutex_lock(&_llogMutex);
    llog->begin(path);
    pthread_mutex_unlock(&_llogMutex);
}

void llogEnd()
{
    lazyinit();
    pthread_mutex_lock(&_llogMutex);
    llog->end();
    pthread_mutex_unlock(&_llogMutex);
}

void llogPut(std::wstring msg)
{
    lazyinit();
    pthread_mutex_lock(&_llogMutex);
    llog->put(msg);
    pthread_mutex_unlock(&_llogMutex);
}

// LiveLogBasic - old ]
// LiveLog common [

void llogLog(std::wstring path, std::wstring msg)
{
    llogLogReplace(path, msg, false);
}

void llogReplace(std::wstring path, std::wstring msg)
{
    llogLogReplace(path, msg, true);
}

void llogLogReplace(std::wstring path, std::wstring msg, bool replace)
{
    lazyinit();

    pthread_mutex_lock(&_llogMutex);

    std::vector<std::wstring> vstrings;

    std::wstringstream ss(path);
    std::wstring item;
    while (std::getline(ss, item, L'/')) {
        vstrings.push_back(item);
        //*(result++) = item;
    }
//    if (vstrings[0] == std::wstring(L"CBlockTreeDB"))
//        return;

    llog->log(path, msg, replace);
    llog->flush();

    pthread_mutex_unlock(&_llogMutex);
}

void llogLog(std::wstring path, std::wstring msg, std::wstring s, bool replace)
{
    std::wostringstream ss;
    ss << msg << " " << s <<"\n";
    llogLogReplace(path, ss.str(), replace);
}

void llogLog(std::wstring path, std::wstring msg, std::string s, bool replace)
{
    std::wostringstream ss;
    ss << msg << " " << s.c_str() <<"\n";
    llogLogReplace(path, ss.str(), replace);
}

void llogLog(std::wstring path, std::wstring msg, const char *s, bool replace)
{
    std::wostringstream ss;
    ss << msg << " " << s <<"\n";
    llogLogReplace(path, ss.str(), replace);
}
/*
void llogLog(std::wstring path, std::wstring msg, int i, bool replace)
{
    std::wostringstream ss;
    ss << msg << " " << i <<"\n";
    llogLogReplace(path, ss.str(), replace);
}*/

void llogLog(std::wstring path, std::wstring msg, int64_t i, bool replace)
{
    std::wostringstream ss;
    ss << msg << " " << i <<"\n";
    llogLogReplace(path, ss.str(), replace);
}

// LiveLog common ]
// LiveLog blockchain [

// _CScriptGetAddresses(CScript &scriptPubKey) [

std::vector<std::string> _CScriptGetAddresses(const CScript &scriptPubKey)
{
    txnouttype type;
    vector <CTxDestination> addresses;
    int nRequired;

    std::vector<std::string> a;

    if (!ExtractDestinations(scriptPubKey, type, addresses, nRequired)) {
        return a;
    }

    BOOST_FOREACH(const CTxDestination& addr, addresses)
        a.push_back(CBitcoinAddress(addr).ToString());

    return a;
}

// _CScriptGetAddresses(CScript &scriptPubKey) ]

void ListTransactions(const CWalletTx& wtx, const string& strAccount, int nMinDepth, bool fLong, Array& ret);

void llogLog(std::wstring path, std::wstring msg, const CBlock &block)
{
    std::wostringstream ss;
    ss << msg << "\n";
    // CBlockHeader
    ss << " nVersion " << block.nVersion << "\n";
    ss << " HASH " << block.GetHash().GetHex().c_str() << "\n";
//    ss << " POW  " << block.GetPoWHash().GetHex().c_str() << "\n";
    ss << " hashPrevBlock " << block.hashPrevBlock.GetHex().c_str() << "\n";
    ss << " hashMerkleRoot " << block.hashMerkleRoot.GetHex().c_str() << "\n";
    ss << " nTime " << block.nTime << "\n";
    // ss << "0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << id;
    ss << " nBits 0x" << std::hex << block.nBits << std::dec << "\n";
    ss << " nNonce " << block.nNonce << "\n";
//    ss << " fChecked " << block.fChecked << "\n";
//    ss << " vchBlockSig size=" << block.vchBlockSig.size() << "\n";
//    std::vector<unsigned char> vchBlockSig;

    for (int i = 0; i < block.vtx.size(); i++) {
//        const CTransactionRef &txRef = block.vtx[i];
//        const CTransaction &tx = *txRef;
        const CTransaction &tx = block.vtx[i];
        ss << " TX " << i << ":\n";
        ss << "  hash " << tx.GetHash().GetHex().c_str() << "\n";
        ss << "  nVersion " << tx.nVersion << "\n";
        ss << "  nTime " << tx.nTime << "\n";
        ss << "  nLockTime " << tx.nLockTime << "\n";

        for (int j = 0; j < tx.vin.size(); j++) {
            const CTxIn &in = tx.vin[j];
            ss << "   IN " << j << ":\n";
            ss << "    nSequence " << in.nSequence << "\n";
            ss << "    prevout " << in.prevout.ToString().c_str() << "\n";
            ss << "    scriptSig size=" << in.scriptSig.size() << "\n";

            ss << "     " << in.scriptSig.ToString().c_str() << "\n";
            ss << "     prevout "<< in.prevout.hash.GetHex().c_str() << "\n";

            const CScript &script = in.scriptSig;

            vector<string> a = _CScriptGetAddresses(script);
            for (int i = 0; i < a.size(); i++)
                ss << "    address " << a[i].c_str() << "\n";

//            CTxDestination& addressRet;
//            ExtractDestination(script, addressRet);


/*            if (pwalletMain) {
                Array details;
                ListTransactions(pwalletMain->mapWallet[tx.GetHash()], "*", 0, false, details);
                for (int i = 0; i < details.size(); i++) {
                    volatile auto a = details[i];
                    i++;
                    i--;
//                volatile auto a = details[i];
//                ss << a;
//                string s = details[i];
//                s=s;
//                Object object = details[i];
//                object=object;
                    //   ss << "    " << details[i].first << " " << details[i].second << "\n";
                }
            }*/
        }
        for (int j = 0; j < tx.vout.size(); j++) {
            const CTxOut &out = tx.vout[j];

            ss << "   OUT " << j << ":\n";
            ss << "    nValue " << out.nValue << "\n";
            ss << "    scriptPubKey size=" << out.scriptPubKey.size() << "\n";
            ss << "     " << out.scriptPubKey.ToString().c_str() << "\n";

            const CScript &script = out.scriptPubKey;

            vector<string> a = _CScriptGetAddresses(script);
            for (int i = 0; i < a.size(); i++)
                ss << "     address " << a[i].c_str() << "\n";
/*
            if (pwalletMain) {
                Array details;
                ListTransactions(pwalletMain->mapWallet[tx.GetHash()], "*", 0, false, details);
                for (int i = 0; i < details.size(); i++) {
                    volatile auto a = details[i];
                    i++;
                    i--;
                }
            }*/
        }
    }
    llogLog(path, ss.str());
}

void llogLog(std::wstring path, std::wstring msg, const CTransaction &tx)
{
    std::wostringstream ss;
    ss << msg << "\n";

    ss << "TX " << ":\n";
    ss << " hash " << tx.GetHash().GetHex().c_str() << "\n";
    ss << " nVersion " << tx.nVersion << "\n";
    ss << " nTime " << tx.nTime << "\n";
    ss << " nLockTime " << tx.nLockTime << "\n";

    for (int j = 0; j < tx.vin.size(); j++) {
        const CTxIn &in = tx.vin[j];
        ss << "  IN " << j << ":\n";
        ss << "   nSequence " << in.nSequence << "\n";
        ss << "   prevout " << in.prevout.ToString().c_str() << "\n";
        ss << "   scriptSig size=" << in.scriptSig.size() << "\n";

        ss << "    " << in.scriptSig.ToString().c_str() << "\n";
        ss << "    prevout "<< in.prevout.hash.GetHex().c_str() << "\n";

        const CScript &script = in.scriptSig;

        vector<string> a = _CScriptGetAddresses(script);
        for (int i = 0; i < a.size(); i++)
            ss << "   address " << a[i].c_str() << "\n";

//            CTxDestination& addressRet;
//            ExtractDestination(script, addressRet);


/*            if (pwalletMain) {
                Array details;
                ListTransactions(pwalletMain->mapWallet[tx.GetHash()], "*", 0, false, details);
                for (int i = 0; i < details.size(); i++) {
                    volatile auto a = details[i];
                    i++;
                    i--;
//                volatile auto a = details[i];
//                ss << a;
//                string s = details[i];
//                s=s;
//                Object object = details[i];
//                object=object;
                    //   ss << "    " << details[i].first << " " << details[i].second << "\n";
                }
            }*/
    }
    for (int j = 0; j < tx.vout.size(); j++) {
        const CTxOut &out = tx.vout[j];

        ss << "  OUT " << j << ":\n";
        ss << "   nValue " << out.nValue << "\n";
        ss << "   scriptPubKey size=" << out.scriptPubKey.size() << "\n";
        ss << "    " << out.scriptPubKey.ToString().c_str() << "\n";

        const CScript &script = out.scriptPubKey;

        vector<string> a = _CScriptGetAddresses(script);
        for (int i = 0; i < a.size(); i++)
            ss << "    address " << a[i].c_str() << "\n";
/*
            if (pwalletMain) {
                Array details;
                ListTransactions(pwalletMain->mapWallet[tx.GetHash()], "*", 0, false, details);
                for (int i = 0; i < details.size(); i++) {
                    volatile auto a = details[i];
                    i++;
                    i--;
                }
            }*/
    }

    llogLog(path, ss.str());
}

void llogLog(std::wstring path, std::wstring msg, const CBlockIndex &blockinfo)
{
    std::wostringstream ss;
    ss << msg << "\n";
    if (&blockinfo) {

        ss << " nHeight " << blockinfo.nHeight << "\n";
//    ss << "GetBlockHash() " << blockinfo.GetBlockHash().GetHex().c_str() << "\n";
//    ss << "GetBlockPoWHash() " << blockinfo.GetBlockPoWHash().GetHex().c_str() << "\n";
        ss << " phashBlock " << blockinfo.phashBlock->GetHex().c_str() << "\n";
        ss << " pprev " << (blockinfo.pprev ? blockinfo.pprev->GetBlockHash().GetHex().c_str() : "NULL") << "\n";
//        ss << " pskip " << (blockinfo.pskip ? blockinfo.pskip->GetBlockHash().GetHex().c_str() : "NULL") << "\n";
        ss << " nFile " << blockinfo.nFile << "\n";
//        ss << " nDataPos " << blockinfo.nDataPos << "\n";
//        ss << " nUndoPos " << blockinfo.nUndoPos << "\n";
//        ss << " nChainWork " << blockinfo.nChainWork.GetHex().c_str() << "\n";
//        ss << " nTx " << blockinfo.nTx << "\n";
//        ss << " nChainTx " << blockinfo.nChainTx << "\n";
//        ss << " nStatus " << blockinfo.nStatus << "\n";
//        ss << " nStakeModifier " << blockinfo.nStakeModifier.GetHex().c_str() << "\n";
        ss << " nVersion " << blockinfo.nVersion << "\n";
        ss << " hashMerkleRoot " << blockinfo.hashMerkleRoot.GetHex().c_str() << "\n";
        ss << " nTime " << blockinfo.nTime << "\n";
        ss << " nBits 0x" << std::hex << blockinfo.nBits << std::dec << "\n";
        ss << " nNonce " << blockinfo.nNonce << "\n";
//        ss << " nSequenceId " << blockinfo.nSequenceId << "\n";
    }
    else
        ss << "blockinfo=NULL";
    llogLog(path, ss.str());
}

// memory
void llogLog(std::wstring path, std::wstring msg, const void *p, int size, int format)
{
    std::wostringstream ss;
    ss << msg << "\n";

    char buf[17];
    int j = 0;

    const uint8_t *p0 = (const uint8_t *)p;

    if (format == 0) {
        ss << std::setfill(L'0') << std::hex;
        int i;
        for (i = 0; i < size; i++) {
            if (i % 16 == 0)
                ss << std::setw(4) << i << ":";
            uint8_t v = *p0++;
            ss << " " << std::setw(2) << v;
            buf[j++] = v < 0x20 || v > 0x7f ? '.' : v;
            if ((i + 1) % 16 == 0) {
                buf[j] = 0;
                j = 0;
                ss << " " << buf << "\n";
            }
        }
        if (i == 0 || i % 16 != 0) {
            buf[j] = 0;
            ss << " " << buf << "\n";
        }

        llogLog(path, ss.str());
    }
}

// LiveLog blockchain ]
