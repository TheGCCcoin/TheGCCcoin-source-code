#ifndef __LLOG_DUMP__
#define __LLOG_DUMP__

#include <string>

class CBlock;
class CBlockIndex;

// setup
void llogSetup(std::string filename = "");

// low level api
void llogBegin(std::wstring path);
void llogEnd();
void llogPut(std::wstring msg);

void llogFlush(bool force);

// high level api
void llogLogReplace(std::wstring path, std::wstring msg, bool replace);
void llogLog(std::wstring path, std::wstring msg);
void llogReplace(std::wstring path, std::wstring msg);

// common
void llogLog(std::wstring path, std::wstring msg, std::wstring s, bool replace=false);
void llogLog(std::wstring path, std::wstring msg, std::string s, bool replace=false);
void llogLog(std::wstring path, std::wstring msg, const char *s, bool replace=false);

// value
//void llogLog(std::wstring path, std::wstring msg, int i, bool replace=false);
void llogLog(std::wstring path, std::wstring msg, int64_t i, bool replace=false);

// blockchain
void llogLog(std::wstring path, std::wstring msg, const CBlock &block);
void llogLog(std::wstring path, std::wstring msg, const CTransaction &tx);
void llogLog(std::wstring path, std::wstring msg, const CBlockIndex &blockinfo);

// util memory
void llogLog(std::wstring path, std::wstring msg, const void *p, int size, int format);

#endif
