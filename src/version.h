// Copyright (c) 2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

#include "clientversion.h"
#include <string>

//
// client versioning
//

static const int CLIENT_VERSION =
                           1000000 * CLIENT_VERSION_MAJOR
                         +   10000 * CLIENT_VERSION_MINOR
                         +     100 * CLIENT_VERSION_REVISION
                         +       1 * CLIENT_VERSION_BUILD;

extern const std::string CLIENT_NAME;
// v.1.1 fix version - todo: review [
//extern const std::string CLIENT_BUILD;
//extern const std::string CLIENT_NUMBERS;
extern std::string CLIENT_BUILD;
extern std::string CLIENT_NUMBERS;
// v.1.1 fix version - todo: review ]
extern const std::string CLIENT_DATE;

//
// network protocol versioning
//

// 62009 : New alerts with easier clearing
//         Different keys for alerts and hash sync checkpoints
// 62010 : New rule to accept duplicate stake on bootstrap (only!)
//         Technically not a network protocol difference
// 64001 : Block version 7
//         Technically not a network protocol difference
static const int PROTOCOL_VERSION = 64001;

//// earlier versions not supported as of Feb 2012, and are disconnected
//static const int MIN_PROTO_VERSION = 61300;

// earlier versions not supported as of Nov 2018, and are disconnected
static const int MIN_PROTO_VERSION = 64000;

// nTime field added to CAddress, starting with this version;
// if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 61001;

// only request blocks from nodes outside this range of versions
static const int NOBLKS_VERSION_START = 0;
static const int NOBLKS_VERSION_END = 62999;

// BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

// "mempool" command, enhanced "getdata" behavior starts with this version:
static const int MEMPOOL_GD_VERSION = 60002;

static const int DATABASE_VERSION = 61201;

#define DISPLAY_VERSION_MAJOR       2
#define DISPLAY_VERSION_MINOR       3
#define DISPLAY_VERSION_REVISION    0
#define DISPLAY_VERSION_BUILD       0

#endif
