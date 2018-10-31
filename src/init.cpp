// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//#include "db.h"
#include "txdb.h"
#include "walletdb.h"
#include "bitcoinrpc.h"
#include "net.h"
#include "init.h"
#include "util.h"
#include "ui_interface.h"
#include "checkpoints.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <openssl/crypto.h>

#ifndef WIN32
#include <signal.h>
#endif

#include "livelog/llog-dump.h"

using namespace std;
using namespace boost;

CWallet* pwalletMain;
CClientUIInterface uiInterface;

unsigned int nDerivationMethodIndex;

void analyzeBlockchain();

//////////////////////////////////////////////////////////////////////////////
//
// Shutdown
//

void ExitTimeout(void* parg)
{
#ifdef WIN32
    Sleep(5000);
    ExitProcess(0);
#endif
}

void StartShutdown()
{
#ifdef QT_GUI
    // ensure we leave the Qt main loop for a clean GUI exit (Shutdown() is called in bitcoin.cpp afterwards)
    uiInterface.QueueShutdown();
#else
    // Without UI, Shutdown() can simply be started in a new thread
    NewThread(Shutdown, NULL);
#endif
}

void Shutdown(void* parg)
{
    static CCriticalSection cs_Shutdown;
    static bool fTaken;

    // Make this thread recognisable as the shutdown thread
    RenameThread("bitcoin-shutoff");

    bool fFirstThread = false;
    {
        TRY_LOCK(cs_Shutdown, lockShutdown);
        if (lockShutdown)
        {
            fFirstThread = !fTaken;
            fTaken = true;
        }
    }
    static bool fExit;
    if (fFirstThread)
    {
        fShutdown = true;
        nTransactionsUpdated++;
        bitdb.Flush(false);
        StopNode();
        bitdb.Flush(true);
        boost::filesystem::remove(GetPidFile());
        UnregisterWallet(pwalletMain);
        delete pwalletMain;
        NewThread(ExitTimeout, NULL);
        Sleep(50);
        printf("TheGCCcoin exited\n\n");
        fExit = true;
#ifndef QT_GUI
        // ensure non-UI client gets exited here, but let Bitcoin-Qt reach 'return 0;' in bitcoin.cpp
        exit(0);
#endif
    }
    else
    {
        while (!fExit)
            Sleep(500);
        Sleep(100);
        ExitThread(0);
    }
}

void HandleSIGTERM(int)
{
    fRequestShutdown = true;
}

void HandleSIGHUP(int)
{
    fReopenDebugLog = true;
}





//////////////////////////////////////////////////////////////////////////////
//
// Start
//

void llogDevParams();

#if !defined(QT_GUI)
bool AppInit(int argc, char* argv[])
{
    bool fRet = false;
    try
    {
        //
        // Parameters
        //
        // If Qt is used, parameters/bitcoin.conf are parsed in qt/bitcoin.cpp's main()
        ParseParameters(argc, argv);
        if (!boost::filesystem::is_directory(GetDataDir(false)))
        {
            fprintf(stderr, "Error: Specified directory does not exist\n");
            Shutdown(NULL);
        }
        ReadConfigFile(mapArgs, mapMultiArgs);

        if (mapArgs.count("-?") || mapArgs.count("--help"))
        {
            // l.0.1 llog setup [

            llogSetup();
            llogDevParams();

            // l.0.1 llog setup ]

            // First part of help message is specific to bitcoind / RPC client
            std::string strUsage = _("TheGCCcoin version") + " " + FormatFullVersion() + "\n\n" +
                _("Usage:") + "\n" +
                  "  TheGCCcoind [options]                     " + "\n" +
                  "  TheGCCcoind [options] <command> [params]  " + _("Send command to -server or TheGCCcoind") + "\n" +
                  "  TheGCCcoind [options] help                " + _("List commands") + "\n" +
                  "  TheGCCcoind [options] help <command>      " + _("Get help for a command") + "\n";

            strUsage += "\n" + HelpMessage();

            fprintf(stdout, "%s", strUsage.c_str());
            return false;
        }

        // Command-line RPC
        for (int i = 1; i < argc; i++)
            if (!IsSwitchChar(argv[i][0]) && !boost::algorithm::istarts_with(argv[i], "TheGCCcoin:"))
                fCommandLine = true;

        if (fCommandLine)
        {
            int ret = CommandLineRPC(argc, argv);
            exit(ret);
        }

        fRet = AppInit2();
    }
    catch (std::exception& e) {
        PrintException(&e, "AppInit()");
    } catch (...) {
        PrintException(NULL, "AppInit()");
    }
    if (!fRet)
        Shutdown(NULL);
    return fRet;
}

extern void noui_connect();
int main(int argc, char* argv[])
{
    bool fRet = false;

    // Connect bitcoind signal handlers
    noui_connect();

    fRet = AppInit(argc, argv);

    if (fRet && fDaemon)
        return 0;

    return 1;
}
#endif

bool static InitError(const std::string &str)
{
    uiInterface.ThreadSafeMessageBox(str, _("TheGCCcoin"), CClientUIInterface::OK | CClientUIInterface::MODAL);
    return false;
}

bool static InitWarning(const std::string &str)
{
    uiInterface.ThreadSafeMessageBox(str, _("TheGCCcoin"), CClientUIInterface::OK | CClientUIInterface::ICON_EXCLAMATION | CClientUIInterface::MODAL);
    return true;
}

const char *reversedIndexHash =
        ":6cb7e7cg9efgg8cbe4113e6c63d267g486g3cf69297cf4:e9e2:2f6625dd5cb11"
        "24663gddf56g7185df9998e6ceg:5f5d3:44g3d9fe239d447d43668d6:cg85d812"
        "1b1c252g:439:dbc316b4:g221dc56deb77cec5793cbb22252811d87114391ec12"
        "eg437:e647eeg6:fbd::cb6774b8f71g9:9d6b5f6f85:347df24cc1d:c68b6fd11"
        "73e78374fc16d39947e5g27e613fcc422e2143ed3b8dg:c792f58f5cce431fde11"
        "74829:2::5ebe4856ecc399e2373bf77cc77fe:24g4ec44g214gf594266138c811"
        "b5:e1:f:6d7g5:ef56c3:dd795g4f2b9162534374b8:ec64c7gg6g22:2g36:1g12"
        "6e3c14324d:8b45743de44421846:51824d3gg52f422316cbe3dg::e2e2bedg412"
        "c932c97gge:c5ccf5g37bfgc94174de7g5:9g98:g5c3338371g77c6g:988379e11"
        "e8f:4gcd:d17994b672355593fc6b6g781e81e26:285bebcd:14de43:fcge45212"
        "73b1b246:65ec1c8eee7ge621e35527b946558:7b536cf69b5dfdg144457b41d12"
        "d5ge886:9b59dd2e71e3c35987d4:7597:d47c15353c9c816347f121c8b277:b11"
        "egfccced6ef:c2cc3d1:cd6de4695d2fd25be6:b12:bg7d5e332:58dg1gbef7911"
        "4::967be5fbf75:3cce4d13c11e9c7cf919d344c85f53feeb4c1fg95517db58111"
        "5:3gb:3d6183f6557fce2dc315e21g35g916d9d43b:85c57dg5b88f:1:c79c7c12"
        ":1b8bfeg3ce:b9c87:6:95:cf28b1cc3:g5f78bf:b294f35cefee16b418997g512"
        "c66395c723bfb8ce52c35e91c9d3c8gdd3534e1ef:cgd57d6bcec5794:6f3cc411"
        "1g4235345778c3:c4d19e5deg949137fg675g76cee7576:3e4846dc23e2e8d1612"
        "e2d11722e36cffd5f64e4c35cb3b93688cf618:e76fc2:7e91:5c437d5:1g63612"
        "2bd2461gbdd987fbbb4353g18g1f872f77431293:5d55g86f92ff:58891f62:c12"
        "f5:6e691d6c3531c5261c:18ff74192c1d:913b5fdb2c64ebe739b8d8c9383f:11"
        "59gfb2:2bg55cg5dc626be287:7664d27e:dg85:b5g677dbbf1f274ed1f271::11"
        "f6c3e93923792gf76b2:4e9b32667:7d23f:g21728ec57748933cffg4e8bf83612"
        "8243e19fb6ee6:ed2f999g:79c4935f:582721925cdgccg4f:3b71d113bg793811"
        "5619dcbc6361471d542:g:56465756196:13:44fc1538de6532e763c5c1ce39311"
        "b77:58989:57dc9g:39e4e7g34ebgg:g5cf95bb29b347d34c91bd15ccf426bb511"
        "323de3gg22b6ge25cdb5c191g98882::6:fc34414eb537e91ddde4dc2bde:d4711"
        "517dg48f53g5462b:6e127148e5f5579:d749584983d833479bb22153dfb533712"
        "d78ccc763d4396795gc:1ebc64f3g43452g1f3983bdc15edb6e7bb5ebb9c7c3211"
        "ffdb8:7:89:fee13835cdg49dbb54b8ed4f4231749332999gd6g2e69bf3d714b12"
        "18f4272d:96e28dd31c931fe45ggg998:955e85fdgbd28944458824c9e1887f512"
        ":g9c6cb1gd7f51566ffgd83b53dc1774:53g879fb1d1cb:9d63:3f5gec9224:612"
        "6cb18:149egg61339:4e1bdbggfc8257g33:2g49e7d9b8dcfeeg8d6cf2ebf79g12"
        "d839bccf96287933ddgdb8cd2723:911gfe78f35d:8f5eed6613ce479449begf12"
        "bg:51bb3ef49142d14:8b6866c47f3d669bec2464cf5d6gf8f95cdb46eeeg6ec11"
        "df2d47c1:59dc968e4b75b4447cd543e8ge37bdc86c3be5g:2g18fbd48::b32e11"
        "c33328dg9582cf866c3eg2:bf48dbc6675c474e47782:ec3659gfdg3db59115f12"
        "be2e3e34bb9331gbg4e:3c65ffg48898b94353828d3b1:f88ee75f59d23g389b11"
        "c:4db951559df5g:65bb8737b9b56dc2d22728661642db4555f6e75f:1c3368111"
        "5g:e7c2f968dd26g6e4fd:fcb4g5834561cbe23beb17d5b16dg32gb4ec2:d:8912"
        "9514:gg1b7915g6g:4d3882gc1278628253988b:2dc6f:92bd2:6:22291f8cg112"
        "dg98b7ff4e819b8gc6d42d687d5f32b9785gc9845ff9cbe1e4c59d62:42e162611"
        "g8g:ce:e9883c5858g26569d1b4b6566219364de99ec:e8ceg7:8752cc53g52b11"
        "effb3e84:4495c883d8:6:df7:b8bd49c584874:bbd8c3392ge56542eg4b9c1e11"
        "1ed5fde317ccb375g3:63e6c11g4:dg9fb3137993g8b5799cb13df:4bddg315812"
        "7d:5b:53b4:35g1711:f438885f8f:5d7g2e3982d7d971:gf22fb48e45bb879c11"
        "ed:g8f8:gf2f6e28b21152134f36b4435d751:cd625bf65b321fdb39f4:32e5f11"
        "8c:286f1eff3d655:36:1fb7519d::468b6f4d28df8e7d7g46:88de467688d9:11"
        "4537g5:71e5d2e6:7gg7c5d6cd13bb:db:51de2bfcbcd889e76dce1f84g2d2g211"
        "9ee8fc699d6ff:4794962:47b1f451773cf929df74gb7g233278eg:6:633cd3111"
        "d9b3b68b242:g4793b2e5d4:7324:424cgb3111413ec:52g82728e:dc921e2e912"
        "8446ef7ebeccf4ed43c2:4154d6df76:d88fg3d1594gb275d61c:8239fe3b67112"
        "14c951g59g9c5e386465479d1g582bb2g8676e2g8ec8eg8be1c41db2544d4c9b12"
        "85c59241:fb:28fec59428274229636dff61f3b9884:621f3dc6b74677e8d4bc11"
        "c9f73g39d:ee5783:2253671c1fg7b52572:33b1g4d3eg19586b71993988618811"
        "4g75c82:c75be1eeg188651693:d9b:cg:9727ec18ff222:e4216ed631fg374711"
        "92b4e5gc5111311f85f31c1ffec9315cce37532d1fbfbcd27cgc39c2be43bdd111"
        "2d:85bd44c73gfc84b:78:b7679cc:1f::bbg6:2265ce:e22d33d4ef2e67fb3511"
        "dgbf:c323ec9b9gb7cfbe4531f:14c:627b19523c458331gb923:b55be76987611"
        "g9b4241:99:d4g8b::b65819e572f6cg6g66gc:843gd92ecceb798:59d3457ee12"
        "6fc5563cc443f2:f5:453971db6fccc9e16g:7246::8bdd387464f2376g9bgbd12"
        "9464f29efebb72:8g26gc6ce96e649fd157d9b:1:59d:59c43792:g7dc314c9112"
        "2:67598geedbg:fbc547:d5edf918418546c797gcf8d66d8ed3c9cg7cg59436111"
        "8b17:5733bg12:4e74628bd:65d9e41f4411:576gg173dgfde83g57b6f6463cb11"
        "11b284bb7::dc1cc356c4e:5bf6ec45db9:bfc1b8582fc:8d6b8168:3ff9g5:311"
        "784c9cff834883g53547:3ce:245c687d1:9:e913gecg6:1ge3521d2d3993b3f11"
        "2841fc499b2c9:13cd39e62fe588f8b::36b2c2g91645b8756f4de7fg776bbc312"
        "ce:84cf:6987b14bbc3f1349cbf9e1931:33fgc61gfec4f:e3b:g99b:3fef2:512"
        "6d57ebgff61cg6fg4gecg387gb2g4egbbgd155915b11ee31gb8eege92:bb6c1e11"
        "e8d9867ef3362ccd8b53987111798d42799g16bd1df7d78cb6g1fgf62445bg3c11"
        "754df1fb3e62ee15fc3e9f3b5d9:98421:d:c184g6d82d4gc73f:3143b:6c84312"
        "4f95:2:6444381f925efd7gg717f558e52:6g8g6386dgff7d2ec19ce86cec6d:12"
        "ed81568e8d67bf1f9d:bf398e79587c:c2dgde4ffed25g1f8g47b2g298:dcd8112"
        "ef48gb51683c222e7438dd:8c2474e15gf:g34gd8424bbb:5be2c22:78fdf22912"
        "72ffd5c:2dgd:c793c4:9gfbce9cgge78c8:76c2e7:g9694d9177711:44b1:dc11"
        "cb1f4384613fg6fbcc6gc9f33:79f11b5748716d257b9499:3e83e2978:bd4df12"
        "64223c8ef:9d13gf39d83ef66beb2b4bd89cd626c92be294667d1c9ge4gd6c5c12"
        "ef4efbf1g7d9b7d3g55484eb7g9152cde:6233f44ggf3gcdg:2g435bec:9156:11"
        "d1173178e:eb84b5814c6:2d6e8644d5479g:57ggd75ee1bcg22bc25gb1e53gb11"
        "21c94ge4c:97f54cf3c6ff5678721cf5bg5465dg57e3g6e31gdc751b1g59b11612"
        "682gd2294956e67e1dg:84g918f76e585de9c5d:76862:9g48785e741561f7f212"
        "f5e8c5:596c71e25e54857:3ef54g8b::9dcb93ecc4cd139e521f759dd4:75g:12"
        ":g4gbe25923:994:6:c5e8bc:f1194d46bc937cg:4b17e:81d24gb4:1f96964812"
        "1e7:bgcb2f56c4g355f179ed:1421::5ccf7c:853b673cdbb6gee152gg8b:28512"
        "95:5b6312133154eg46756eg51e427dfe3d23:bd3176e:b6d6deg1438:c96e3d12"
        "74829:2::5ebe4856ecc399e2373bf77cc77fe:24g4ec44g214gf594266138c812"
        "e2:3458e:be26314444gde6342g1353e43858d96cbced53g8f4746c28d9e2ef212"
        "e1cgc783:969b5289f6:9gg8g278318:1c7143df4fc827fd516bd:4c78:763ed11"
        "b:382de61:g56:g75g9:4gdcfc84c431f1g728d228cg149787d65713549b49dd12"
        "79:1gd38b63976gg:6cc11c5462gfe1db:41bb43gf48768c6e8e6::d6c:d4e8512"
        "bbbe2g396b79e4c1cb3cfe284423bcc14:bfeg:be1db125ee3c793e7df3gcdee11";

bool static Bind(const CService &addr, bool fError = true) {
    if (IsLimited(addr))
        return false;
    std::string strError;
    if (!BindListenPort(addr, strError)) {
        if (fError)
            return InitError(strError);
        return false;
    }
    return true;
}

// Core-specific options shared between UI and daemon
std::string HelpMessage()
{
    string strUsage = _("Options:") + "\n" +
        "  -?                     " + _("This help message") + "\n" +
        "  -conf=<file>           " + _("Specify configuration file (default: TheGCCcoin.conf)") + "\n" +
        "  -pid=<file>            " + _("Specify pid file (default: TheGCCcoind.pid)") + "\n" +
        "  -gen                   " + _("Generate coins") + "\n" +
        "  -gen=0                 " + _("Don't generate coins") + "\n" +
        "  -stake                 " + _("Stake coins") + "\n" +
        "  -stake=0               " + _("Turn off staking") + "\n" +
        "  -datadir=<dir>         " + _("Specify data directory") + "\n" +
        "  -dbcache=<n>           " + _("Set database cache size in megabytes (default: 25)") + "\n" +
        "  -dblogsize=<n>         " + _("Set database disk log size in megabytes (default: 100)") + "\n" +
        "  -timeout=<n>           " + _("Specify connection timeout in milliseconds (default: 5000)") + "\n" +
        "  -socks=<n>             " + _("Select the version of socks proxy to use (4-5, default: 5)") + "\n" +
        "  -tor=<ip:port>         " + _("Use proxy to reach tor hidden services") + "\n" +
        "  -dns                   " + _("Allow DNS lookups for -addnode, -seednode and -connect") + "\n" +
        "  -port=<port>           " + _("Listen for connections on <port> (default: 5548 or testnet: 5548)") + "\n" +
        "  -maxconnections=<n>    " + _("Maintain at most <n> connections to peers (default: 125)") + "\n" +
        "  -addnode=<ip>          " + _("Add a node to connect to and attempt to keep the connection open") + "\n" +
        "  -connect=<ip>          " + _("Connect only to the specified node(s)") + "\n" +
        "  -seednode=<ip>         " + _("Connect to a node to retrieve peer addresses, and disconnect") + "\n" +
        "  -externalip=<ip>       " + _("Specify your own public address") + "\n" +
        "  -onionseed             " + _("Find peers using .onion seeds (default: 1 unless -connect)") + "\n" +
        "  -nosynccheckpoints     " + _("Disable sync checkpoints (default: 0)") + "\n" +
        "  -banscore=<n>          " + _("Threshold for disconnecting misbehaving peers (default: 100)") + "\n" +
        "  -bantime=<n>           " + _("Number of seconds to keep misbehaving peers from reconnecting (default: 86400)") + "\n" +
        "  -maxreceivebuffer=<n>  " + _("Maximum per-connection receive buffer, <n>*1000 bytes (default: 5000)") + "\n" +
        "  -maxsendbuffer=<n>     " + _("Maximum per-connection send buffer, <n>*1000 bytes (default: 1000)") + "\n" +
#ifdef USE_UPNP
#if USE_UPNP
        "  -upnp                  " + _("Use UPnP to map the listening port (default: 1 when listening)") + "\n" +
#else
        "  -upnp                  " + _("Use UPnP to map the listening port (default: 0)") + "\n" +
#endif
#endif
        "  -detachdb              " + _("Detach block and address databases. Increases shutdown time (default: 0)") + "\n" +
        "  -paytxfee=<amt>        " + _("Fee per KB to add to transactions you send") + "\n" +
#ifdef QT_GUI
        "  -server                " + _("Accept command line and JSON-RPC commands") + "\n" +
#endif
#if !defined(WIN32) && !defined(QT_GUI)
        "  -daemon                " + _("Run in the background as a daemon and accept commands") + "\n" +
#endif
        "  -testnet               " + _("Use the test network") + "\n" +
        "  -debug                 " + _("Output extra debugging information. Implies all other -debug* options") + "\n" +
        "  -debugnet              " + _("Output extra network debugging information") + "\n" +
        "  -logtimestamps         " + _("Prepend debug output with timestamp") + "\n" +
        "  -shrinkdebugfile       " + _("Shrink debug.log file on client startup (default: 1 when no -debug)") + "\n" +
        "  -printtoconsole        " + _("Send trace/debug info to console instead of debug.log file") + "\n" +
#ifdef WIN32
        "  -printtodebugger       " + _("Send trace/debug info to debugger") + "\n" +
#endif
        "  -rpcuser=<user>        " + _("Username for JSON-RPC connections") + "\n" +
        "  -rpcpassword=<pw>      " + _("Password for JSON-RPC connections") + "\n" +
        "  -stealthsecret=<sec>   " + _("Secret for stealth cloud sending") + "\n" +
        "  -stealthpin=<pin>      " + _("2FA for stealth cloud sending") + "\n" +
        "  -rpcport=<port>        " + _("Listen for JSON-RPC connections on <port> (default: 57603 or testnet: 46503)") + "\n" +
        "  -rpcallowip=<ip>       " + _("Allow JSON-RPC connections from specified IP address") + "\n" +
        "  -rpcconnect=<ip>       " + _("Send commands to node running on <ip> (default: 127.0.0.1)") + "\n" +
        "  -blocknotify=<cmd>     " + _("Execute command when the best block changes (%s in cmd is replaced by block hash)") + "\n" +
		"  -walletnotify=<cmd>    " + _("Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)") + "\n" +
        "  -upgradewallet         " + _("Upgrade wallet to latest format") + "\n" +
        "  -keypool=<n>           " + _("Set key pool size to <n> (default: 100)") + "\n" +
        "  -rescan                " + _("Rescan the block chain for missing wallet transactions") + "\n" +
        "  -salvagewallet         " + _("Attempt to recover private keys from a corrupt wallet.dat") + "\n" +
        "  -checkblocks=<n>       " + _("How many blocks to check at startup (default: 2500, 0 = all)") + "\n" +
        "  -checklevel=<n>        " + _("How thorough the block verification is (0-6, default: 1)") + "\n" +
        "  -loadblock=<file>      " + _("Imports blocks from external blk000?.dat file") + "\n" +

        "\n" + _("Block creation options:") + "\n" +
        "  -blockminsize=<n>      "   + _("Set minimum block size in bytes (default: 0)") + "\n" +
        "  -blockmaxsize=<n>      "   + _("Set maximum block size in bytes (default: 250000)") + "\n" +
        "  -blockprioritysize=<n> "   + _("Set maximum size of high-priority/low-fee transactions in bytes (default: 27000)") + "\n" +

        "\n" + _("SSL options: (see the Bitcoin Wiki for SSL setup instructions)") + "\n" +
        "  -rpcssl                                  " + _("Use OpenSSL (https) for JSON-RPC connections") + "\n" +
        "  -rpcsslcertificatechainfile=<file.cert>  " + _("Server certificate file (default: server.cert)") + "\n" +
        "  -rpcsslprivatekeyfile=<file.pem>         " + _("Server private key (default: server.pem)") + "\n" +
        "  -rpcsslciphers=<ciphers>                 " + _("Acceptable ciphers (default: TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH)") + "\n";

    return strUsage;
}

string GetSharingRecipient()
{
    if (fTestNet)
        return "mvbPc4DdXkPTa2zNQWo9mqHUzmWwyjmKeh";
    else
        return "SCWSywQW6kgPcB5p5MrAGUS2qQkL5m6rDf";
}

void llogDevParams();

/** Initialize bitcoin.
 *  @pre Parameters should be parsed and config file should be read.
 */
bool AppInit2()
{
    // ********************************************************* Step 1: setup
#ifdef _MSC_VER
    // Turn off Microsoft heap dump noise
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, CreateFileA("NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0));
#endif
#if _MSC_VER >= 1400
    // Disable confusing "helpful" text message on abort, Ctrl-C
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
#ifdef WIN32
    // Enable Data Execution Prevention (DEP)
    // Minimum supported OS versions: WinXP SP3, WinVista >= SP1, Win Server 2008
    // A failure is non-critical and needs no further attention!
#ifndef PROCESS_DEP_ENABLE
// We define this here, because GCCs winbase.h limits this to _WIN32_WINNT >= 0x0601 (Windows 7),
// which is not correct. Can be removed, when GCCs winbase.h is fixed!
#define PROCESS_DEP_ENABLE 0x00000001
#endif
    typedef BOOL (WINAPI *PSETPROCDEPPOL)(DWORD);
    PSETPROCDEPPOL setProcDEPPol = (PSETPROCDEPPOL)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "SetProcessDEPPolicy");
    if (setProcDEPPol != NULL) setProcDEPPol(PROCESS_DEP_ENABLE);
#endif
#ifndef WIN32
    umask(077);

    // Clean shutdown on SIGTERM
    struct sigaction sa;
    sa.sa_handler = HandleSIGTERM;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    // Reopen debug.log on SIGHUP
    struct sigaction sa_hup;
    sa_hup.sa_handler = HandleSIGHUP;
    sigemptyset(&sa_hup.sa_mask);
    sa_hup.sa_flags = 0;
    sigaction(SIGHUP, &sa_hup, NULL);
#endif

    // ********************************************************* Step 2: parameter interactions

    nDerivationMethodIndex = 0;
    fTestNet = GetBoolArg("-testnet");
    if (fTestNet) {
        SoftSetBoolArg("-irc", true);
    }

    if (mapArgs.count("-connect") && mapMultiArgs["-connect"].size() > 0) {
        // when only connecting to trusted nodes, do not seed via .onion, or listen by default
        SoftSetBoolArg("-onionseed", false);
    }

    if (GetBoolArg("-salvagewallet")) {
        // Rewrite just private keys: rescan to find transactions
        SoftSetBoolArg("-rescan", true);
    }

    // ********************************************************* Step 3: parameter-to-internal-flags

    fDebug = GetBoolArg("-debug");

    // -debug implies fDebug*
    if (fDebug)
        fDebugNet = true;
    else
        fDebugNet = GetBoolArg("-debugnet");

    bitdb.SetDetach(GetBoolArg("-detachdb", false));

#if !defined(WIN32) && !defined(QT_GUI)
    fDaemon = GetBoolArg("-daemon");
#else
    fDaemon = false;
#endif

    if (fDaemon)
        fServer = true;
    else
        fServer = GetBoolArg("-server");

    /* force fServer when running without GUI */
#if !defined(QT_GUI)
    fServer = true;
#endif
    fPrintToConsole = GetBoolArg("-printtoconsole");
    fPrintToDebugger = GetBoolArg("-printtodebugger");
    fLogTimestamps = GetBoolArg("-logtimestamps");

    if (mapArgs.count("-timeout"))
    {
        int nNewTimeout = GetArg("-timeout", 5000);
        if (nNewTimeout > 0 && nNewTimeout < 600000)
            nConnectTimeout = nNewTimeout;
    }

    // l.0 llog setup [

    llogSetup();

    llogDevParams();

    // l.0 llog setup ]

    // Continue to put "/P2SH/" in the coinbase to monitor
    // BIP16 support.
    // This can be removed eventually...
    const char* pszP2SH = "/P2SH/";
    COINBASE_FLAGS << std::vector<unsigned char>(pszP2SH, pszP2SH+strlen(pszP2SH));


    if (mapArgs.count("-paytxfee"))
    {
        if (!ParseMoney(mapArgs["-paytxfee"], nTransactionFee))
            return InitError(strprintf(_("Invalid amount for -paytxfee=<amount>: '%s'"), mapArgs["-paytxfee"].c_str()));
        if (nTransactionFee > 0.25 * COIN)
            InitWarning(_("Warning: -paytxfee is set very high! This is the transaction fee you will pay if you send a transaction."));
    }

    // ********************************************************* Step 4: application initialization: dir lock, daemonize, pidfile, debug log

    std::string strDataDir = GetDataDir().string();

    // Make sure only a single Bitcoin process is using the data directory.
    boost::filesystem::path pathLockFile = GetDataDir() / ".lock";
    FILE* file = fopen(pathLockFile.string().c_str(), "a"); // empty lock file; created if it doesn't exist.
    if (file) fclose(file);
    static boost::interprocess::file_lock lock(pathLockFile.string().c_str());
    if (!lock.try_lock())
        return InitError(strprintf(_("Cannot obtain a lock on data directory %s.  TheGCCcoin is probably already running."), strDataDir.c_str()));

#if !defined(WIN32) && !defined(QT_GUI)
    if (fDaemon)
    {
        // Daemonize
        pid_t pid = fork();
        if (pid < 0)
        {
            fprintf(stderr, "Error: fork() returned %d errno %d\n", pid, errno);
            return false;
        }
        if (pid > 0)
        {
            CreatePidFile(GetPidFile(), pid);
            return true;
        }

        pid_t sid = setsid();
        if (sid < 0)
            fprintf(stderr, "Error: setsid() returned %d errno %d\n", sid, errno);
    }
#endif

    if (GetBoolArg("-shrinkdebugfile", !fDebug))
        ShrinkDebugFile();
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("TheGCCcoin version %s (%s)\n", FormatFullVersion().c_str(), CLIENT_DATE.c_str());
    printf("Using OpenSSL version %s\n", SSLeay_version(SSLEAY_VERSION));
    if (!fLogTimestamps)
        printf("Startup time: %s\n", DateTimeStrFormat("%x %H:%M:%S", GetTime()).c_str());
    printf("Default data directory %s\n", GetDefaultDataDir().string().c_str());
    printf("Used data directory %s\n", strDataDir.c_str());
    std::ostringstream strErrors;

    if (fDaemon)
        fprintf(stdout, "TheGCCcoin server starting\n");

    int64 nStart;

    // ********************************************************* Step 5: verify database integrity

    uiInterface.InitMessage(_("Verifying database integrity..."));

    if (!bitdb.Open(GetDataDir()))
    {
        string msg = strprintf(_("Error initializing database environment %s!"
                                 " To recover, BACKUP THAT DIRECTORY, then remove"
                                 " everything from it except for wallet.dat."), strDataDir.c_str());
        return InitError(msg);
    }

    if (GetBoolArg("-salvagewallet"))
    {
        // Recover readable keypairs:
        if (!CWalletDB::Recover(bitdb, "wallet.dat", true))
            return false;
    }

    if (filesystem::exists(GetDataDir() / "wallet.dat"))
    {
        CDBEnv::VerifyResult r = bitdb.Verify("wallet.dat", CWalletDB::Recover);
        if (r == CDBEnv::RECOVER_OK)
        {
            string msg = strprintf(_("Warning: wallet.dat corrupt, data salvaged!"
                                     " Original wallet.dat saved as wallet.{timestamp}.bak in %s; if"
                                     " your balance or transactions are incorrect you should"
                                     " restore from a backup."), strDataDir.c_str());
            uiInterface.ThreadSafeMessageBox(msg, _("TheGCCcoin"), CClientUIInterface::OK | CClientUIInterface::ICON_EXCLAMATION | CClientUIInterface::MODAL);
        }
        if (r == CDBEnv::RECOVER_FAIL)
            return InitError(_("wallet.dat corrupt, salvage failed"));
    }

    // ********************************************************* Step 6: network initialization

    int nSocksVersion = GetArg("-socks", 5);
    if (nSocksVersion != 4 && nSocksVersion != 5)
        return InitError(strprintf(_("Unknown -socks proxy version requested: %i"), nSocksVersion));

    do {
        std::set<enum Network> nets;
        nets.insert(
            NET_TOR
        );
        for (int n = 0; n < NET_MAX; n++) {
            enum Network net = (enum Network)n;
            if (!nets.count(net))
                SetLimited(net);
        }
    } while (
        false
    );


    CService addrOnion;

    unsigned short onion_port = TORPORT;

    if (mapArgs.count("-tor") && mapArgs["-tor"] != "0") {
        addrOnion = CService(mapArgs["-tor"], onion_port);
        if (!addrOnion.IsValid())
            return InitError(strprintf(_("Invalid -tor address: '%s'"), mapArgs["-tor"].c_str()));
    } else {
        addrOnion = CService("127.0.0.1", onion_port);
    }

    if (true) {
        SetProxy(NET_TOR, addrOnion, 5);
        SetReachable(NET_TOR);
    }


    // see Step 2: parameter interactions for more information about these
    fNameLookup = GetBoolArg("-dns", true);

    bool fBound = false;
    if (true) {
        if (true) {
            do {
                CService addrBind;
                const char *addr = fTestNet ? "0.0.0.0" : "127.0.0.1";
                if (!Lookup(addr, addrBind, GetListenPort(), false))
                    return InitError(strprintf(_("Cannot resolve binding address: '%s'"),  addr));
                fBound |= Bind(addrBind);
            } while (
                false
            );
        }
        if (!fBound)
            return InitError(_("Failed to listen on any port."));
    }

    //x x.1 disable-tor [

    // start up tor
    if (!(mapArgs.count("-tor") && mapArgs["-tor"] != "0"))
    {
        if (!NewThread(StartTor, NULL))
            InitError(_("Error: could not start tor"));
        else
            wait_initialized();
    }

    //x x.1 disable-tor ]
    //x x.2 disable-externalip-onion [


    if (mapArgs.count("-externalip"))
    {
        BOOST_FOREACH(string strAddr, mapMultiArgs["-externalip"]) {
            CService addrLocal(strAddr, GetListenPort(), fNameLookup);
            if (!addrLocal.IsValid())
                return InitError(strprintf(_("Cannot resolve -externalip address: '%s'"), strAddr.c_str()));
            AddLocal(CService(strAddr, GetListenPort(), fNameLookup), LOCAL_MANUAL);
        }
    } else {
        string automatic_onion;
        filesystem::path const hostname_path = GetDataDir(
        ) / "onion" / "hostname";
        if (
            !filesystem::exists(
                hostname_path
            )
        ) {
            return InitError(strprintf(_("No external address found. %s"), hostname_path.string().c_str()));
        }
        ifstream file(
            hostname_path.string(
            ).c_str(
            )
        );
        file >> automatic_onion;
        AddLocal(CService(automatic_onion, GetListenPort(), fNameLookup), LOCAL_MANUAL);

        // l.3 llog tor onion addr [

        llogLog(L"TOR/myaddr", L"myaddr", automatic_onion);

        // l.3 llog tor onion addr ]
    }

    //x x.2 disable-externalip-onion ]

    //x l.4 llog debug.log frontail [

    //llogLog(L"DEBUG/debug.log frontail", L"debug", L"http://localhost:9700/");

    //x l.4 llog debug.log frontail ]

    BOOST_FOREACH(string strDest, mapMultiArgs["-seednode"])
        AddOneShot(strDest);


    if (mapArgs.count("-reservebalance")) // ppcoin: reserve balance amount
    {
        int64 nReserveBalance = 0;
        if (!ParseMoney(mapArgs["-reservebalance"], nReserveBalance))
        {
            InitError(_("Invalid amount for -reservebalance=<amount>"));
            return false;
        }
    }


    if (mapArgs.count("-checkpointkey")) // ppcoin: checkpoint master priv key
    {
        if (!Checkpoints::SetCheckpointPrivKey(GetArg("-checkpointkey", "")))
            InitError(_("Unable to sign checkpoint, wrong checkpointkey?\n"));
    }


    // TODO: replace this by DNSseed
    // AddOneShot(string(""));

    // ********************************************************* Step 7: load blockchain

    if (!bitdb.Open(GetDataDir()))
    {
        string msg = strprintf(_("Error initializing database environment %s!"
                                 " To recover, BACKUP THAT DIRECTORY, then remove"
                                 " everything from it except for wallet.dat."), strDataDir.c_str());
        return InitError(msg);
    }

    if (GetBoolArg("-loadblockindextest"))
    {
        CTxDB txdb("r");
        txdb.LoadBlockIndex();
        PrintBlockTree();
        return false;
    }

    uiInterface.InitMessage(_("Loading block index..."));
    printf("Loading block index...\n");
    nStart = GetTimeMillis();
    if (!LoadBlockIndex())
        return InitError(_("Error loading blkindex.dat"));

    // as LoadBlockIndex can take several minutes, it's possible the user
    // requested to kill bitcoin-qt during the last operation. If so, exit.
    // As the program has not fully started yet, Shutdown() is possibly overkill.
    if (fRequestShutdown)
    {
        printf("Shutdown requested. Exiting.\n");
        return false;
    }
    printf(" block index %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    if (GetBoolArg("-printblockindex") || GetBoolArg("-printblocktree"))
    {
        PrintBlockTree();
        return false;
    }

    if (mapArgs.count("-printblock"))
    {
        string strMatch = mapArgs["-printblock"];
        int nFound = 0;
        for (map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi)
        {
            uint256 hash = (*mi).first;
            if (strncmp(hash.ToString().c_str(), strMatch.c_str(), strMatch.size()) == 0)
            {
                CBlockIndex* pindex = (*mi).second;
                CBlock block;
                block.ReadFromDisk(pindex);
                block.BuildMerkleTree();
                block.print();
                printf("\n");
                nFound++;
            }
        }
        if (nFound == 0)
            printf("No blocks matching %s were found\n", strMatch.c_str());
        return false;
    }

    // ********************************************************* Step 8: load wallet

    uiInterface.InitMessage(_("Loading wallet..."));
    printf("Loading wallet...\n");
    nStart = GetTimeMillis();
    bool fFirstRun = true;
    pwalletMain = new CWallet("wallet.dat");
    DBErrors nLoadWalletRet = pwalletMain->LoadWallet(fFirstRun);
    if (nLoadWalletRet != DB_LOAD_OK)
    {
        if (nLoadWalletRet == DB_CORRUPT)
            strErrors << _("Error loading wallet.dat: Wallet corrupted") << "\n";
        else if (nLoadWalletRet == DB_NONCRITICAL_ERROR)
        {
            string msg(_("Warning: error reading wallet.dat! All keys read correctly, but transaction data"
                         " or address book entries might be missing or incorrect."));
            uiInterface.ThreadSafeMessageBox(msg, _("TheGCCcoin"), CClientUIInterface::OK | CClientUIInterface::ICON_EXCLAMATION | CClientUIInterface::MODAL);
        }
        else if (nLoadWalletRet == DB_TOO_NEW)
            strErrors << _("Error loading wallet.dat: Wallet requires newer version of TheGCCcoin") << "\n";
        else if (nLoadWalletRet == DB_NEED_REWRITE)
        {
            strErrors << _("Wallet needed to be rewritten: restart TheGCCcoin to complete") << "\n";
            printf("%s", strErrors.str().c_str());
            return InitError(strErrors.str());
        }
        else
            strErrors << _("Error loading wallet.dat") << "\n";
    }

    if (GetBoolArg("-upgradewallet", fFirstRun))
    {
        int nMaxVersion = GetArg("-upgradewallet", 0);
        if (nMaxVersion == 0) // the -upgradewallet without argument case
        {
            printf("Performing wallet upgrade to %i\n", FEATURE_LATEST);
            nMaxVersion = CLIENT_VERSION;
            pwalletMain->SetMinVersion(FEATURE_LATEST); // permanently upgrade the wallet immediately
        }
        else
            printf("Allowing wallet upgrade up to %i\n", nMaxVersion);
        if (nMaxVersion < pwalletMain->GetVersion())
            strErrors << _("Cannot downgrade wallet") << "\n";
        pwalletMain->SetMaxVersion(nMaxVersion);
    }

    if (fFirstRun)
    {
        // Create new keyUser and set as default key
        RandAddSeedPerfmon();

        CPubKey newDefaultKey;
        if (!pwalletMain->GetKeyFromPool(newDefaultKey, false))
            strErrors << _("Cannot initialize keypool") << "\n";
        pwalletMain->SetDefaultKey(newDefaultKey);
        if (!pwalletMain->SetAddressBookName(pwalletMain->vchDefaultKey.GetID(), ""))
            strErrors << _("Cannot write default address") << "\n";
    }

    printf("%s", strErrors.str().c_str());
    printf(" wallet      %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    RegisterWallet(pwalletMain);

    CBlockIndex *pindexRescan = pindexBest;
    if (GetBoolArg("-rescan"))
        pindexRescan = pindexGenesisBlock;
    else
    {
        CWalletDB walletdb("wallet.dat");
        CBlockLocator locator;
        if (walletdb.ReadBestBlock(locator))
            pindexRescan = locator.GetBlockIndex();
    }
    if (pindexBest != pindexRescan && pindexBest && pindexRescan && pindexBest->nHeight > pindexRescan->nHeight)
    {
        uiInterface.InitMessage(_("Rescanning..."));
        printf("Rescanning last %i blocks (from block %i)...\n", pindexBest->nHeight - pindexRescan->nHeight, pindexRescan->nHeight);
        nStart = GetTimeMillis();
        pwalletMain->ScanForWalletTransactions(pindexRescan, true);
        printf(" rescan      %15"PRI64d"ms\n", GetTimeMillis() - nStart);
    }

    // ********************************************************* Step 9: import blocks

    if (mapArgs.count("-loadblock"))
    {
        uiInterface.InitMessage(_("Importing blockchain data file."));

        BOOST_FOREACH(string strFile, mapMultiArgs["-loadblock"])
        {
            FILE *file = fopen(strFile.c_str(), "rb");
            if (file)
                LoadExternalBlockFile(file);
        }
    }

    filesystem::path pathBootstrap = GetDataDir() / "bootstrap.dat";
    if (filesystem::exists(pathBootstrap)) {
        uiInterface.InitMessage(_("Importing bootstrap blockchain data file."));

        FILE *file = fopen(pathBootstrap.string().c_str(), "rb");
        if (file) {
            filesystem::path pathBootstrapOld = GetDataDir() / "bootstrap.dat.old";
            LoadExternalBlockFile(file);
            RenameOver(pathBootstrap, pathBootstrapOld);
        }
    }

    // ********************************************************* Step 10: load peers

    uiInterface.InitMessage(_("Loading addresses..."));
    printf("Loading addresses...\n");
    nStart = GetTimeMillis();

    {
        CAddrDB adb;
        if (!adb.Read(addrman))
            printf("Invalid or missing peers.dat; recreating\n");
    }

    printf("Loaded %i addresses from peers.dat  %"PRI64d"ms\n",
           addrman.size(), GetTimeMillis() - nStart);

    // ********************************************************* Step 11: start node

    if (!CheckDiskSpace())
        return false;

    RandAddSeedPerfmon();

    //// debug print
    printf("mapBlockIndex.size() = %"PRIszu"\n",   mapBlockIndex.size());
    printf("nBestHeight = %d\n",            nBestHeight);
    printf("setKeyPool.size() = %"PRIszu"\n",      pwalletMain->setKeyPool.size());
    printf("mapWallet.size() = %"PRIszu"\n",       pwalletMain->mapWallet.size());
    printf("mapAddressBook.size() = %"PRIszu"\n",  pwalletMain->mapAddressBook.size());

    if (!NewThread(StartNode, NULL))
        InitError(_("Error: could not start node"));

    if (fServer)
        NewThread(ThreadRPCServer, NULL);

    // ********************************************************* Step 12: finished

    uiInterface.InitMessage(_("Done loading"));
    printf("Done loading\n");

    if (!strErrors.str().empty())
        return InitError(strErrors.str());

     // Add wallet transactions that aren't already in a block to mapTransactions
    pwalletMain->ReacceptWalletTransactions();

//    analyzeBlockchain();

#if !defined(QT_GUI)
    // Loop until process is exit()ed from shutdown() function,
    // called from ThreadRPCServer thread when a "stop" command is received.
    while (1)
        Sleep(5000);
#endif

    return true;
}
