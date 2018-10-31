#include "Profile.cpp"
#include "livelog/llog-dump.h"

using namespace acpul;
using namespace std;

Profile profile;

// crawler [

// CrawlerContext [

class CrawlerContext {
public:
    map<uint256, CBlock> &mblocks;
    map<uint256, CBlock *> mtxblock;
    map<COutPoint, CBlock *> &mvinblock;
    map<COutPoint, CBlock *> &mvoutblock;
    map<COutPoint, const CTransaction *> mvintx;
    map<COutPoint, const CTransaction *> mvouttx;
    map<COutPoint, int64> &voutvalue;

    uint256 scanBlockHash;
    int64 scanBlockValue;
    int scanOuts;

    map<COutPoint, bool> voutScanned;

    // block [

    uint256 blockHash;
    CBlock *block;
    CBlockIndex *pblockIndex;

    // block ]

    CrawlerContext(
            map<uint256, CBlock> &mblocks_,
            map<uint256, CBlock *> &mtxblock_,
            map<COutPoint, CBlock *> &mvinblock_,
            map<COutPoint, CBlock *> &mvoutblock_,
            map<COutPoint, const CTransaction *> mvintx_,
            map<COutPoint, const CTransaction *> mvouttx_,
            map<COutPoint, int64> &voutvalue_
    )
    : mblocks(mblocks_)
    , mtxblock(mtxblock_)
    , mvinblock(mvinblock_)
    , mvoutblock(mvoutblock_)
    , mvintx(mvintx_)
    , mvouttx(mvouttx_)
    , voutvalue(voutvalue_)
    {
    }

    void selectTx(const CTransaction &tx)
    {
        block = mtxblock[tx.GetHash()];
        blockHash = block->GetHash();
        pblockIndex = mapBlockIndex[blockHash];
    }

    int64 totalValue;
    int totalOuts;

    vector<string> pathStack;

    void push()
    {
        pathStack.push_back(blockHash.GetHex());
    }

    void pop()
    {
        if (pathStack.size() > 0)
            pathStack.pop_back();
    }

    wstring getPath()
    {
        string s;
        for (int i = 0; i < pathStack.size(); i++)
            s += ((i > 0) ? "/" : "") + pathStack[i];

        wstring ws(s.begin(), s.end());
        return ws;
    }
};

// CrawlerContext ]
// def [

void scanBlocks(CrawlerContext &ctx, vector<uint256> &blocks);
void scanBlock(CrawlerContext &ctx, uint256 hash, bool onlyCoinBase);
void scanTx(CrawlerContext &ctx, const CTransaction &tx);

// def ]
// scanBlocks [

void scanBlocks(CrawlerContext &ctx, vector<uint256> &blocks, bool onlyCoinBase)
{
    ctx.totalValue = 0;
    ctx.totalOuts = 0;

    llogLog(L"Analyze/crawl/catched!", L"");

    for (int k = 14; k < blocks.size(); k++) {
        uint256 hash = blocks[k];

        if (k==15)
            break;

        ctx.scanBlockHash = hash;
        ctx.scanBlockValue = 0;
        ctx.scanOuts = 0;

        scanBlock(ctx, hash, onlyCoinBase);

        std::wostringstream ss;
        ss << "block (" << k << ") " << hash.GetHex().c_str();
        ss << " https://blockchain.thegcccoin.com/block/" << hash.GetHex().c_str() << "\n";
        ss << " blockValue " << ctx.scanBlockValue/1000000 << "   " << ctx.scanBlockValue << "\n";
        ss << " totalValue " << ctx.totalValue/1000000 << "   " << ctx.totalValue << "\n";
        ss << " blockOuts " << ctx.scanOuts << "\n";
        ss << " totalOuts " << ctx.totalOuts << "\n";
        llogLog(L"Analyze/crawl/catched!", ss.str());
        llogFlush(true);
    }
}

// scanBlocks ]
// scanBlock [

void scanBlock(CrawlerContext &ctx, uint256 hash, bool onlyCoinBase)
{
    CBlock &block = ctx.mblocks[hash];

    for (int i = 0; i < block.vtx.size(); i++) {
        const CTransaction &tx = block.vtx[i];

        // check if pow only [

        if (onlyCoinBase) {
            if (!tx.vin[0].prevout.IsNull())
                continue;
        }

        // check if pow only ]

        scanTx(ctx, tx);
    }
}

// scanBlock ]
// scanTx [

void scanTx(CrawlerContext &ctx, const CTransaction &tx)
{
    ctx.selectTx(tx);

    for (int j = 0; j < tx.vout.size(); j++) {
        const CTxOut &out = tx.vout[j];

        // find (vin ==> vout) -> block [

        uint256 txhash = tx.GetHash();

        COutPoint outpoint(txhash, j);

        if (ctx.voutScanned[outpoint])
            continue;

        ctx.voutScanned[outpoint] = true;

        CBlock *blockFrom = ctx.mvinblock[outpoint];
        const CTransaction *txFrom = ctx.mvintx[outpoint];

        // find (vin ==> vout) -> block ]
        // process outpoint [

        if (blockFrom) {
            ctx.push();

            std::wostringstream ss;
            ss << "out " << outpoint.hash.GetHex().c_str() << "-" << outpoint.n;// << "\n";
            ss << " https://blockchain.thegcccoin.com/tx/" << outpoint.hash.GetHex().c_str() << "\n";
            ss << " value " << std::setprecision(12) << std::fixed << ((double)ctx.voutvalue[outpoint]/1000000.) << "\n";
            CBlock *block = ctx.mvoutblock[outpoint];
            if (block) {
                uint256 hash = block->GetHash();
                CBlockIndex *pblockIndex = mapBlockIndex[hash];
                if (pblockIndex)
                    ss << " height " << pblockIndex->nHeight << "\n";
                ss << " block " << hash.GetHex().c_str();// << "\n";
                ss << " https://blockchain.thegcccoin.com/block/" << block->GetHash().GetHex().c_str() << "\n";
            }
            ss << " -> " << blockFrom->GetHash().GetHex().c_str();// << "\n";
            ss << " https://blockchain.thegcccoin.com/block/" << blockFrom->GetHash().GetHex().c_str() << "\n";

            ss << " -> tx " << txFrom->GetHash().GetHex().c_str();
            ss << " https://blockchain.thegcccoin.com/tx/" << txFrom->GetHash().GetHex().c_str() << "\n";

            std::wostringstream path;
            path << "*** " << ctx.scanBlockHash.GetHex().c_str();
            llogLog(L"Analyze/crawl/spent/" + path.str(), ss.str());
            llogFlush(true);

            scanTx(ctx, *txFrom);

            ctx.pop();
        }
        else {
            std::wostringstream ss;
            if ((double)ctx.voutvalue[outpoint]/1000000. > 10.) {
//                if ((double)ctx.voutvalue[outpoint]/1000000. > 5000.) {
               // if (true) {//(double)ctx.voutvalue[outpoint]/1000000. > 500000.) {
//            if ((double)ctx.voutvalue[outpoint]/1000000. > 50000.) {
                ss << "out (" << ctx.scanOuts << ") " << outpoint.hash.GetHex().c_str() << "-" << outpoint.n;// << "\n";
                ss << " https://blockchain.thegcccoin.com/tx/" << outpoint.hash.GetHex().c_str() << "\n";
//                ss << " height " << ctx.pblockIndex->nHeight << "\n";

                ss << " block " << ctx.mtxblock[txhash]->GetHash().GetHex().c_str() << " " << ctx.pblockIndex->nHeight;
                ss << " https://blockchain.thegcccoin.com/block/" << ctx.mtxblock[txhash]->GetHash().GetHex().c_str()
                   << "\n";

                ss << " value " << std::setprecision(12) << std::fixed
                   << ((double) ctx.voutvalue[outpoint] / 1000000.);// << "\n";
                ss << " (+" << std::setprecision(12) << std::fixed << ((double) ctx.scanBlockValue / 1000000.) << ")\n";

//            ss << "https://blockchain.thegcccoin.com/rawtx/" << outpoint.hash.GetHex().c_str() << "\n";
//            ss << "https://blockchain.thegcccoin.com/tx/" << outpoint.hash.GetHex().c_str() << "\n";
//            ss << "https://blockchain.thegcccoin.com/block/" << ctx.mtxblock[txhash]->GetHash().GetHex().c_str() << "\n";

                std::wostringstream path;
                path << "*** " << ctx.scanBlockHash.GetHex().c_str();
//                llogLog(L"Analyze/crawl/unspent/" + path.str() + L"/" + ctx.getPath(), ss.str());

                std::wostringstream ss1;
                ss1 << outpoint.hash.GetHex().c_str() << "-" << outpoint.n << "\n";
                llogLog(L"Analyze/crawl/bad-outputs", ss1.str());
//                ctx.scanBlockAcceptValue += ctx.voutvalue[outpoint];
                ctx.scanBlockValue += ctx.voutvalue[outpoint];
                ctx.totalValue += ctx.voutvalue[outpoint];
            }
            else {
                ss << "out (" << ctx.scanOuts << ") " << outpoint.hash.GetHex().c_str() << "-" << outpoint.n << " ";// << "\n";
                ss << ((double) ctx.voutvalue[outpoint] / 1000000.) << " ";// << "\n";
                ss << " https://blockchain.thegcccoin.com/tx/" << outpoint.hash.GetHex().c_str() << "\n";
                llogLog(L"Analyze/crawl/skip-low-value", ss.str());
            }

            llogFlush(true);

            // process outpoint ]

//            ctx.scanBlockValue += ctx.voutvalue[outpoint];
            ctx.totalOuts++;
            ctx.scanOuts++;
        }
    }
}

// scanTx ]

// crawler ]
// analyzeBlockchain [

void analyzeBlockchain()
{
    // find all pow blocks

    int nHeight = 500000;
    int startHeight = 995801;

    int blockIndexCount = mapBlockIndex.size();

    // load blocks [

    profile.begin("all-blocks");
    profile.begin("blocks-per-second");

    map<uint256, CBlock> mblocks;

    vector<uint256> powBlocks;
    vector<uint256> posBlocks;

    int64 powTxsCount = 0;
    int64 posTxsCount = 0;

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hashBestChain];

    while (pblockindex->nHeight > startHeight) {
        pblockindex = pblockindex->pprev;
    }

    while (pblockindex->nHeight > nHeight) {
        int h = pblockindex->nHeight;
        pblockindex = pblockindex->pprev;
        CBlock block;
        block.ReadFromDisk(pblockindex, true);

        uint256 hash = *pblockindex->phashBlock;
        mblocks[hash] = block;

        if (block.IsProofOfStake()) {
            posBlocks.push_back(hash);
            posTxsCount += block.vtx.size();
        }
        else if (block.IsProofOfWork()) {
            powBlocks.push_back(hash);
            powTxsCount += block.vtx.size();
        }

        // p.1 llog profile blocks [

        int n = 10000;
        if (h % n == 0) {
            ProfileInfo info = profile.end("blocks-per-second");
            profile.begin("blocks-per-second");

            std::wostringstream ss;
            ss << "height=" << h << "\n";
            double t = info.dt / 1000000;
            ss << "n=" << n << " " << n/t << " blocks/second " << t << " sec \n";
            llogReplace(L"Analyze/AllBlocks", ss.str());
        }

        // p.1 llog profile blocks ]
    }

    ProfileInfo info = profile.end("all-blocks");
    llogLog(L"Analyze/AllBlocks", L"load blocks time", info.dt/1000000);

    // load blocks ]
    // process blocks [

    profile.begin("process-blocks");

    uint256 hashNull; // == 0

    int64 powValue = 0;
    int64 powZeroCount = 0;

    map<int64, int64> mvaluesCount;

    map<COutPoint, int64> voutvalue;

    // process pow [

    vector<uint256> powBlocksSelected;

    for (int k = 0; k < powBlocks.size(); k++) {
        uint256 hash = powBlocks[k];
        CBlock &block = mblocks[hash];

        for (int i = 0; i < block.vtx.size(); i++) {
            const CTransaction &tx = block.vtx[i];

            uint256 txhash = tx.GetHash();

            // verify valid pow [

            for (int j = 0; j < tx.vin.size(); j++) {
                const CTxIn &in = tx.vin[j];
                const COutPoint &prevout = in.prevout;

                if (prevout.hash != hashNull && i == 0) {
                    llogLog(L"Analyze/AllBlocks", L"bad pow prevout != null", hash.GetHex());
                }
            }

            // verify valid pow ]
            // select  & stats pow blocks - wrong if value > 1 coin [

            for (int j = 0; j < tx.vout.size(); j++) {
                const CTxOut &out = tx.vout[j];

                if (i == 0) {
                    int64 value = out.nValue;
                    double v = (double)value / 1000000.;
                    if (v <= 1.)
                        powZeroCount++;
                    else {
                        std::wostringstream ss;
                        CBlockIndex *pindex = mapBlockIndex[hash];
                        ss << "height=" << pindex->nHeight << " " << hash.GetHex().c_str() << " " << std::setprecision(12) << std::fixed << v << "\n";
                        llogLog(L"Analyze/$Blocks", ss.str());
//                        llogLog(L"Analyze/$Blocks", L"block", hash.GetHex());

                        powBlocksSelected.push_back(hash);
                    }
                    mvaluesCount[value] += 1;
                    powValue += value;
                }
            }

            // select  & stats pow blocks - wrong if value > 1 coin ]
        }
    }

    // process pow ]

    info = profile.end("process-blocks");
    llogLog(L"Analyze/AllBlocks", L"process blocks time", info.dt/1000000);

    llogLog(L"Analyze/AllBlocks", L"pos blocks", posBlocks.size());
    llogLog(L"Analyze/AllBlocks", L"pow blocks", powBlocks.size());
    llogLog(L"Analyze/AllBlocks", L"pos txs", posTxsCount);
    llogLog(L"Analyze/AllBlocks", L"pow txs", powTxsCount);
    llogLog(L"Analyze/AllBlocks", L"pow value mined", powValue);
    llogLog(L"Analyze/AllBlocks", L"pow value mined", powValue/1000000);
    llogLog(L"Analyze/AllBlocks", L"pow zero value blocks", powZeroCount);
    llogLog(L"Analyze/AllBlocks", L"pow non-zero value blocks", powBlocks.size() - powZeroCount);
    llogLog(L"Analyze/AllBlocks", L"pow values count", mvaluesCount.size());

    // process blocks ]
    // build tx in-out index [

    profile.begin("build tx in-out");

    map<uint256, CBlock *> mtxblock;
    map<COutPoint, CBlock *> mvinblock;
    map<COutPoint, CBlock *> mvoutblock;
    map<COutPoint, const CTransaction *> mvintx;
    map<COutPoint, const CTransaction *> mvouttx;

    int64 totalvoutValue = 0;

    for (auto it = mblocks.begin(); it != mblocks.end(); it++) {
        CBlock &block = it->second;

        for (int i = 0; i < block.vtx.size(); i++) {
            const CTransaction &tx = block.vtx[i];

            uint256 txhash = tx.GetHash();

            mtxblock[txhash] = &block;

            for (int j = 0; j < tx.vin.size(); j++) {
                const CTxIn &in = tx.vin[j];
                const COutPoint &prevout = in.prevout;
                mvinblock[prevout] = &block;
                mvintx[prevout] = &tx;
            }
            for (int j = 0; j < tx.vout.size(); j++) {
                const CTxOut &out = tx.vout[j];

                COutPoint outpoint(txhash, j);
                mvoutblock[outpoint] = &block;
                mvouttx[outpoint] = &tx;

                // value
                int64 value = out.nValue;
                voutvalue[outpoint] = value;

                totalvoutValue += value;
            }
        }
    }

    info = profile.end("build tx in-out");
    llogLog(L"Analyze/AllBlocks", L"build tx in-out time", info.dt/1000000);

    llogLog(L"Analyze/AllBlocks", L"mtxblock count", mtxblock.size());
    llogLog(L"Analyze/AllBlocks", L"mvinblock count", mvinblock.size());
    llogLog(L"Analyze/AllBlocks", L"mvoutblock count", mvoutblock.size());
    llogLog(L"Analyze/AllBlocks", L"voutvalue count", voutvalue.size());
    llogLog(L"Analyze/AllBlocks", L"totalvoutValue", totalvoutValue);
    llogLog(L"Analyze/AllBlocks", L"totalvoutValue", totalvoutValue/1000000);


    // build tx in-out index ]
    // crawl pow tx -> all childs [

    profile.begin("crawl tx");

    // for powBlocksSelected as block [

    for (int k = 0; k < powBlocksSelected.size(); k++) {
        uint256 hash = powBlocksSelected[k];
        CBlock &block = mblocks[hash];

        CBlockIndex* pblockindex = mapBlockIndex[hash];

        // for tx in block [

        for (int i = 0; i < block.vtx.size(); i++) {
            const CTransaction &tx = block.vtx[i];

            // process tx [

            // pow only!!! [

            // is coin base
            if (!tx.vin[0].prevout.IsNull())
                continue;

            // pow only!!! ]

            uint256 txhash = tx.GetHash();

            // for vout in tx [

//            crawlTransaction()
            for (int j = 0; j < tx.vout.size(); j++) {
                const CTxOut &out = tx.vout[j];

                // process vout [

                // find (vin ==> vout) -> block [

                COutPoint outpoint(txhash, j);
                CBlock *block = mvinblock[outpoint];

                // find (vin ==> vout) -> block ]

                // : block not found [

                if (!block) {
                    std::wostringstream ss;
                    ss << "out " << outpoint.hash.GetHex().c_str() << "-" << outpoint.n << " height=" << pblockindex->nHeight << " " << voutvalue[outpoint] << " " << hash.GetHex().c_str() << "\n";
                    llogLog(L"Analyze/tx/out->in not found", ss.str());
                    continue;
                }

                // : block not found ]
                // : block found [

                std::wostringstream ss;
//                ss << "out " << outpoint.hash.GetHex().c_str() << "-" << outpoint.n << " -> block" << hash.GetHex().c_str() << "\n";
                ss << "out " << outpoint.hash.GetHex().c_str() << "-" << outpoint.n << " height=" << pblockindex->nHeight << " " << voutvalue[outpoint] << " -> block " << block->GetHash().GetHex().c_str() << "\n";

                llogLog(L"Analyze/tx/out->in", ss.str());

                int vins = 0, vouts = 0;
                for (int i = 0; i < block->vtx.size(); i++) {
                    vins += block->vtx[i].vin.size();
                    vouts += block->vtx[i].vout.size();
                }
                ss << "vtxs=" << block->vtx.size() << " vins=" << vins << " vouts=" << vouts << "\n";
                llogLog(L"Analyze/tx/out->in block", ss.str());
//                llogLog(L"Analyze/tx/out->in block", L"block", *block);
//                llogLog(L"Analyze/tx/out->in block", L"block", *block);

                // : block found ]

                // process vout ]

            }

            // for vout in tx ]

/*            for (int j = 0; j < tx.vin.size(); j++) {
                const CTxIn &in = tx.vin[j];
                const COutPoint &prevout = in.prevout;

                CBlock *block = moutpointblock[prevout];
                if (block) {
                }
            }
            }*/


            // process tx ]

        }

        // for tx in block ]
    }

    // for powBlocksSelected as block ]

    info = profile.end("crawl tx");
    llogLog(L"Analyze/AllBlocks", L"crawl tx time", info.dt/1000000);

    // crawl pow tx -> all childs ]

    for (auto it = mvaluesCount.begin(); it != mvaluesCount.end(); it++) {
        std::wostringstream ss;
        ss << std::setprecision(12) << std::fixed << it->first << " * " << it->second << " = " << ((double)(it->first * it->second) / 1000000) << "\n";
        llogLog(L"Analyze/$values", ss.str());
    }

    // do crawl it! [

    CrawlerContext ctx(mblocks, mtxblock, mvinblock, mvoutblock, mvintx, mvouttx, voutvalue);

    scanBlocks(ctx, powBlocksSelected, true);

    // do crawl it! ]
}

// analyzeBlockchain ]

