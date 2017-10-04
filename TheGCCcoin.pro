TEMPLATE = app
TARGET = "TheGCCcoin Qt"
VERSION = 2.2.3.2

INCLUDEPATH += src src/json src/qt src/tor
INCLUDEPATH += src/tor/adapter src/tor/common src/tor/ext
INCLUDEPATH += src/tor/ext/curve25519_donna src/tor/or

DEFINES += QT_GUI BOOST_THREAD_USE_LIB BOOST_SPIRIT_THREADSAFE BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN __NO_SYSTEM_INCLUDES
DEFINES += QT_GUI BOOST_THREAD_USE_LIB BOOST_SPIRIT_THREADSAFE

QT += core gui network widgets webkit

CONFIG += no_include_pwd
CONFIG += thread
CONFIG += exceptions

lessThan(QT_MAJOR_VERSION, 5): CONFIG += webkit
QMAKE_CXXFLAGS = -fpermissive

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += webkit webkitwidgets
    DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
}

win32 {
DEPS_PATH=d:/GCC/deps_mingw
BOOST_LIB_SUFFIX=-mgw49-mt-s-1_64
BOOST_INCLUDE_PATH=$$DEPS_PATH/boost_1_64_0
BOOST_LIB_PATH=$$DEPS_PATH/boost_1_64_0/stage/lib
BDB_INCLUDE_PATH=$$DEPS_PATH/db-4.8.30.NC/build_unix
BDB_LIB_PATH=$$DEPS_PATH/db-4.8.30.NC/build_unix/.libs
OPENSSL_INCLUDE_PATH=$$DEPS_PATH/openssl-1.0.1l/include
OPENSSL_LIB_PATH=$$DEPS_PATH/openssl-1.0.1l
MINIUPNPC_INCLUDE_PATH=$$DEPS_PATH
MINIUPNPC_LIB_PATH=$$DEPS_PATH/miniupnpc
QRENCODE_INCLUDE_PATH=$$DEPS_PATH/qrencode-3.4.4
QRENCODE_LIB_PATH=$$DEPS_PATH/qrencode-3.4.4/.libs
LIBEVENT_INCLUDE_PATH=$$DEPS_PATH/libevent-2.1.8-stable/include
LIBEVENT_LIB_PATH=$$DEPS_PATH/libevent-2.1.8-stable/.libs
CRYPTO_INCLUDE_PATH=$$DEPS_PATH
CRYPTO_LIB_PATH=$$DEPS_PATH/cryptopp
LEVELDB_INCLUDE_PATH=$$DEPS_PATH/leveldb/include
LEVELDB_HELPER_INCLUDE_PATH=$$DEPS_PATH/leveldb/helpers
LEVELDB_LIB_PATH=$$DEPS_PATH/leveldb

INCLUDEPATH += $$BOOST_INCLUDE_PATH $$BDB_INCLUDE_PATH $$OPENSSL_INCLUDE_PATH $$MINIUPNPC_INCLUDE_PATH $$QRENCODE_INCLUDE_PATH $$LIBEVENT_INCLUDE_PATH $$CRYPTO_INCLUDE_PATH $$LEVELDB_INCLUDE_PATH $$LEVELDB_HELPER_INCLUDE_PATH
LIBS += $$join(BOOST_LIB_PATH,,-L,) $$join(BDB_LIB_PATH,,-L,) $$join(OPENSSL_LIB_PATH,,-L,) $$join(MINIUPNPC_LIB_PATH,,-L,) $$join(QRENCODE_LIB_PATH,,-L,) $$join(LIBEVENT_LIB_PATH,,-L,) $$join(CRYPTO_LIB_PATH,,-L,) $$join(LEVELDB_LIB_PATH,,-L,)
}
else {
DEPS_PATH=/opt/local
BOOST_LIB_SUFFIX=-mt-s
INCLUDE_PATH=$$DEPS_PATH/include
LIB_PATH=$$DEPS_PATH/lib

INCLUDEPATH += $$INCLUDE_PATH
LIBS += $$join(LIB_PATH,,-L,)
}

OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build

# for extra security (see: https://wiki.debian.org/Hardening): this flag is GCC compiler-specific
QMAKE_CXXFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2
win32:QMAKE_LFLAGS *= -Wl,--dynamicbase -Wl,--nxcompat
# on win32: enable GCC large address aware linker flag
win32:QMAKE_LFLAGS *= -Wl,--large-address-aware

# use: qmake "USE_DBUS=1"
contains(USE_DBUS, 1) {
    message(Building with DBUS (Freedesktop notifications) support)
    DEFINES += USE_DBUS
    QT += dbus
}

# use: qmake "USE_IPV6=1" ( enabled by default; default)
#  or: qmake "USE_IPV6=0" (disabled by default)
#  or: qmake "USE_IPV6=-" (not supported)
contains(USE_IPV6, -) {
    message(Building without IPv6 support)
} else {
    count(USE_IPV6, 0) {
        USE_IPV6=1
    }
    DEFINES += USE_IPV6=$$USE_IPV6
}

# use: qmake "FIRST_CLASS_MESSAGING=1"
contains(FIRST_CLASS_MESSAGING, 1) {
    message(Building with first-class messaging)
    DEFINES += FIRST_CLASS_MESSAGING
}

contains(BITCOIN_NEED_QT_PLUGINS, 1) {
    DEFINES += BITCOIN_NEED_QT_PLUGINS
    QTPLUGIN += qcncodecs qjpcodecs qtwcodecs qkrcodecs qtaccessiblewidgets
}

# use: qmake "RELEASE=1"
contains(RELEASE, 1) {
    # Mac: ensure compatibility with at least 10.7, 64 bit
    macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.6 -arch i386 -isysroot /Developer/SDKs/MacOSX10.6.sdk
    macx:QMAKE_CFLAGS += -mmacosx-version-min=10.6 -arch i386 -isysroot /Developer/SDKs/MacOSX10.6.sdk
    macx:QMAKE_OBJECTIVE_CFLAGS += -mmacosx-version-min=10.6 -arch i386 -isysroot /Developer/SDKs/MacOSX10.6.sdk

    !win32:!macx {
        # Linux: static link
        LIBS += -Wl,-Bstatic -Wl,-z,relro -Wl,-z,now
    }
}

USE_QRCODE=1
# use: qmake "USE_QRCODE=1"
# libqrencode (http://fukuchi.org/works/qrencode/index.en.html) must be installed for support
contains(USE_QRCODE, 1) {
    message(Building with QRCode support)
    DEFINES += USE_QRCODE
    LIBS += -lqrencode
}

#USE_UPNP=-
# use: qmake "USE_UPNP=1" ( enabled by default; default)
#  or: qmake "USE_UPNP=0" (disabled by default)
#  or: qmake "USE_UPNP=-" (not supported)
# miniupnpc (http://miniupnp.free.fr/files/) must be installed for support
contains(USE_UPNP, -) {
    message(Building without UPNP support)
} else {
    message(Building with UPNP support)
    count(USE_UPNP, 0) {
        USE_UPNP=1
    }
    DEFINES += USE_UPNP=$$USE_UPNP STATICLIB
    INCLUDEPATH += $$MINIUPNPC_INCLUDE_PATH
    LIBS += $$join(MINIUPNPC_LIB_PATH,,-L,) -lminiupnpc
    win32:LIBS += -liphlpapi
}

!win32 {
    # for extra security against potential buffer overflows
    QMAKE_CXXFLAGS += -fstack-protector
    QMAKE_LFLAGS += -fstack-protector
    # do not enable this on windows, as it will result in a non-working executable!
}

# regenerate src/build.h
!windows|contains(USE_BUILD_INFO, 1) {
    genbuild.depends = FORCE
    genbuild.commands = cd $$PWD; /bin/sh share/genbuild.sh $$OUT_PWD/build/build.h
    genbuild.target = $$OUT_PWD/build/build.h
    PRE_TARGETDEPS += $$OUT_PWD/build/build.h
    QMAKE_EXTRA_TARGETS += genbuild
    DEFINES += HAVE_BUILD_INFO
}

contains(USE_O3, 1) {
    message(Building without O3 optimization flag)
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS += -O3
    QMAKE_CFLAGS += -O3
}

*-g++-32 {
    message("32 platform, adding -msse2 flag")

    QMAKE_CXXFLAGS += -msse2
    QMAKE_CFLAGS += -msse2
}

QMAKE_CXXFLAGS_WARN_ON = -fdiagnostics-show-option -Wall -Wextra -Wformat -Wformat-security -Wno-unused-parameter

# Input
DEPENDPATH += src src/json src/qt

HEADERS += \
    src/stealthaddress.h \
    src/qt/bitcoingui.h \
    src/qt/transactiontablemodel.h \
    src/qt/addresstablemodel.h \
    src/qt/optionsdialog.h \
    src/qt/coincontroldialog.h \
    src/qt/coincontroltreewidget.h \
    src/qt/sendcoinsdialog.h \
    src/qt/addressbookpage.h \
    src/qt/signverifymessagedialog.h \
    src/qt/aboutdialog.h \
    src/qt/editaddressdialog.h \
    src/qt/bitcoinaddressvalidator.h \
    src/alert.h \
    src/addrman.h \
    src/base58.h \
    src/bignum.h \
    src/checkpoints.h \
    src/compat.h \
    src/coincontrol.h \
    src/sync.h \
    src/util.h \
    src/uint256.h \
    src/kernel.h \
    src/scrypt_mine.h \
    src/pbkdf2.h \
    src/serialize.h \
    src/strlcpy.h \
    src/main.h \
    src/net.h \
    src/key.h \
    src/db.h \
    src/txdb.h \
    src/txdb-leveldb.h \
    src/walletdb.h \
    src/script.h \
    src/init.h \
    src/irc.h \
    src/mruset.h \
    src/json/json_spirit_writer_template.h \
    src/json/json_spirit_writer.h \
    src/json/json_spirit_value.h \
    src/json/json_spirit_utils.h \
    src/json/json_spirit_stream_reader.h \
    src/json/json_spirit_reader_template.h \
    src/json/json_spirit_reader.h \
    src/json/json_spirit_error_position.h \
    src/json/json_spirit.h \
    src/qt/clientmodel.h \
    src/qt/guiutil.h \
    src/qt/transactionrecord.h \
    src/qt/guiconstants.h \
    src/qt/optionsmodel.h \
    src/qt/monitoreddatamapper.h \
    src/qt/transactiondesc.h \
    src/qt/transactiondescdialog.h \
    src/qt/bitcoinamountfield.h \
    src/wallet.h \
    src/keystore.h \
    src/qt/transactionfilterproxy.h \
    src/qt/walletmodel.h \
    src/bitcoinrpc.h \
    src/qt/overviewpage.h \
    src/qt/csvmodelwriter.h \
    src/crypter.h \
    src/qt/sendcoinsentry.h \
    src/qt/qvalidatedlineedit.h \
    src/qt/bitcoinunits.h \
    src/qt/qvaluecombobox.h \
    src/qt/askpassphrasedialog.h \
    src/protocol.h \
    src/qt/notificator.h \
    src/qt/qtipcserver.h \
    src/allocators.h \
    src/ui_interface.h \
    src/qt/rpcconsole.h \
    src/version.h \
    src/netbase.h \
    src/clientversion.h \
    src/hashblock.h \
    src/sph_blake.h \
    src/sph_bmw.h \
    src/sph_cubehash.h \
    src/sph_echo.h \
    src/sph_groestl.h \
    src/sph_jh.h \
    src/sph_keccak.h \
    src/sph_luffa.h \
    src/sph_shavite.h \
    src/sph_simd.h \
    src/sph_skein.h \
    src/sph_fugue.h \
    src/sph_hamsi.h \
    src/sph_types.h \
    src/stealthtext.h \
    src/qt/httpsocket.h \
    src/qt/qcircleprogressbar.h \
    src/qt/qstealthsplash.h \
    src/qt/qstealthgrid.h \
    src/qt/qhoverbutton.h \
    src/qt/qbottombar.h \
    src/qt/common/qstealth.h \
    src/qt/qstealthprogressbar.h \
    src/qt/qtitlepopup.h \
    src/qt/qtitlebar.h \
    src/qt/qstealthmain.h \
    src/qt/qsidemenubar.h \
    src/qt/qsidebutton.h \
    src/qt/qpagesendxst.h \
    src/qt/qpageoverview.h \
    src/qt/blockbrowser.h \
    src/qt/qpageaddrbook.h \
    src/qt/qgridbutton.h \
    src/qt/qstealthpage.h \
    src/qt/qstealthtableview.h \
    src/qt/common/mymodel.h \
    src/qt/qpagetransactions.h

SOURCES += \
    src/qt/bitcoin.cpp \
    src/qt/bitcoingui.cpp \
    src/qt/transactiontablemodel.cpp \
    src/qt/addresstablemodel.cpp \
    src/qt/optionsdialog.cpp \
    src/qt/sendcoinsdialog.cpp \
    src/qt/coincontroldialog.cpp \
    src/qt/coincontroltreewidget.cpp \
    src/qt/addressbookpage.cpp \
    src/qt/signverifymessagedialog.cpp \
    src/qt/aboutdialog.cpp \
    src/qt/editaddressdialog.cpp \
    src/qt/bitcoinaddressvalidator.cpp \
    src/tor/common/address.c \
    src/tor/or/addressmap.c \
    src/tor/common/aes.c \
    src/tor/common/backtrace.c \
    src/tor/or/buffers.c \
    src/tor/or/channel.c \
    src/tor/or/channeltls.c \
    src/tor/or/circpathbias.c \
    src/tor/or/circuitbuild.c \
    src/tor/or/circuitlist.c \
    src/tor/or/circuitmux.c \
    src/tor/or/circuitmux_ewma.c \
    src/tor/or/circuitstats.c \
    src/tor/or/circuituse.c \
    src/tor/or/command.c \
    src/tor/common/tor_compat.c \
    src/tor/common/compat_libevent.c \
    src/tor/or/config.c \
    src/tor/or/config_codedigest.c \
    src/tor/or/confparse.c \
    src/tor/or/connection.c \
    src/tor/or/connection_edge.c \
    src/tor/or/connection_or.c \
    src/tor/common/container.c \
    src/tor/or/control.c \
    src/tor/or/cpuworker.c \
    src/tor/common/crypto.c \
    src/tor/common/crypto_curve25519.c \
    src/tor/common/crypto_format.c \
    src/tor/ext/csiphash.c \
    src/tor/ext/curve25519_donna/curve25519-donna.c \
    src/tor/common/di_ops.c \
    src/tor/or/directory.c \
    src/tor/or/dirserv.c \
    src/tor/or/dirvote.c \
    src/tor/or/dns.c \
    src/tor/or/dnsserv.c \
    src/tor/or/entrynodes.c \
    # src/tor/ext/eventdns.c \
    src/tor/or/ext_orport.c \
    src/tor/or/fp_pair.c \
    src/tor/or/geoip.c \
    src/tor/or/hibernate.c \
    src/tor/common/log.c \
    src/tor/common/memarea.c \
    src/tor/common/mempool.c \
    src/tor/or/microdesc.c \
    src/tor/or/networkstatus.c \
    src/tor/or/nodelist.c \
    src/tor/or/onion.c \
    src/tor/or/onion_fast.c \
    src/tor/or/onion_main.c \
    src/tor/or/onion_ntor.c \
    src/tor/or/onion_tap.c \
    src/tor/or/policies.c \
    src/tor/adapter/stealth.cpp \
    src/tor/common/procmon.c \
    src/tor/or/reasons.c \
    src/tor/or/relay.c \
    src/tor/or/rendclient.c \
    src/tor/or/rendcommon.c \
    src/tor/or/rendmid.c \
    src/tor/or/rendservice.c \
    src/tor/or/rephist.c \
    src/tor/or/replaycache.c \
    src/tor/or/router.c \
    src/tor/or/routerlist.c \
    src/tor/or/routerparse.c \
    src/tor/or/routerset.c \
    src/tor/common/sandbox.c \
    src/tor/or/statefile.c \
    src/tor/or/status.c \
    src/tor/common/tor_util.c \
    src/tor/common/torgzip.c \
    src/tor/common/tortls.c \
    src/tor/or/tor_main.c \
    src/tor/or/transports.c \
    src/tor/common/util_codedigest.c \
    src/tor/common/util_process.c \
    src/alert.cpp \
    src/version.cpp \
    src/sync.cpp \
    src/util.cpp \
    src/netbase.cpp \
    src/key.cpp \
    src/script.cpp \
    src/main.cpp \
    src/init.cpp \
    src/net.cpp \
    src/irc.cpp \
    src/checkpoints.cpp \
    src/addrman.cpp \
    src/db.cpp \
    src/txdb-leveldb.cpp \
    src/walletdb.cpp \
    src/stealthaddress.cpp \
    src/qt/clientmodel.cpp \
    src/qt/guiutil.cpp \
    src/qt/transactionrecord.cpp \
    src/qt/optionsmodel.cpp \
    src/qt/monitoreddatamapper.cpp \
    src/qt/transactiondesc.cpp \
    src/qt/transactiondescdialog.cpp \
    src/qt/bitcoinstrings.cpp \
    src/qt/bitcoinamountfield.cpp \
    src/wallet.cpp \
    src/keystore.cpp \
    src/qt/transactionfilterproxy.cpp \
    src/qt/walletmodel.cpp \
    src/bitcoinrpc.cpp \
    src/rpcdump.cpp \
    src/rpcnet.cpp \
    src/rpcmining.cpp \
    src/rpcwallet.cpp \
    src/rpcblockchain.cpp \
    src/rpcrawtransaction.cpp \
    src/qt/overviewpage.cpp \
    src/qt/csvmodelwriter.cpp \
    src/crypter.cpp \
    src/qt/sendcoinsentry.cpp \
    src/qt/qvalidatedlineedit.cpp \
    src/qt/bitcoinunits.cpp \
    src/qt/qvaluecombobox.cpp \
    src/qt/askpassphrasedialog.cpp \
    src/protocol.cpp \
    src/qt/notificator.cpp \
    src/qt/qtipcserver.cpp \
    src/qt/rpcconsole.cpp \
    src/noui.cpp \
    src/kernel.cpp \
    src/scrypt-x86.S \
    src/scrypt-x86_64.S \
    src/scrypt_mine.cpp \
    src/pbkdf2.cpp \
    src/aes_helper.c \
    src/blake.c \
    src/bmw.c \
    src/cubehash.c \
    src/echo.c \
    src/groestl.c \
    src/jh.c \
    src/keccak.c \
    src/luffa.c \
    src/shavite.c \
    src/simd.c \
    src/skein.c \
    src/fugue.c \
    src/hamsi.c \
    src/scrypt.cpp \
    src/stealthtext.cpp \
    src/qt/httpsocket.cpp \
    src/qt/qcircleprogressbar.cpp \
    src/qt/qstealthsplash.cpp \
    src/qt/qstealthgrid.cpp \
    src/qt/qhoverbutton.cpp \
    src/qt/qbottombar.cpp \
    src/qt/common/qstealth.cpp \
    src/qt/qstealthprogressbar.cpp \
    src/qt/qtitlepopup.cpp \
    src/qt/qtitlebar.cpp \
    src/qt/qstealthmain.cpp \
    src/qt/qsidemenubar.cpp \
    src/qt/qsidebutton.cpp \
    src/qt/qpagesendxst.cpp \
    src/qt/qpageoverview.cpp \
    src/qt/blockbrowser.cpp \
    src/qt/qpageaddrbook.cpp \
    src/qt/qgridbutton.cpp \
    src/qt/qstealthpage.cpp \
    src/qt/qstealthtableview.cpp \
    src/qt/common/mymodel.cpp \
    src/qt/qpagetransactions.cpp

RESOURCES += \
    src/qt/bitcoin.qrc

FORMS += \
    src/qt/forms/coincontroldialog.ui \
    src/qt/forms/sendcoinsdialog.ui \
    src/qt/forms/addressbookpage.ui \
    src/qt/forms/signverifymessagedialog.ui \
    src/qt/forms/aboutdialog.ui \
    src/qt/forms/editaddressdialog.ui \
    src/qt/forms/transactiondescdialog.ui \
    src/qt/forms/overviewpage.ui \
    src/qt/forms/sendcoinsentry.ui \
    src/qt/forms/askpassphrasedialog.ui \
    src/qt/forms/rpcconsole.ui \
    src/qt/forms/optionsdialog.ui \
    src/qt/forms/blockbrowser.ui

win32 {
  HEADERS += src/tor/or/ntmain.h
}


win32 {
  SOURCES += src/tor/or/ntmain.c
}

contains(USE_QRCODE, 1) {
HEADERS += src/qt/qrcodedialog.h
SOURCES += src/qt/qrcodedialog.cpp
FORMS += src/qt/forms/qrcodedialog.ui
}

contains(BITCOIN_QT_TEST, 1) {
SOURCES += src/qt/test/test_main.cpp \
    src/qt/test/uritests.cpp
HEADERS += src/qt/test/uritests.h
DEPENDPATH += src/qt/test
QT += testlib
TARGET = fun-qt_test
DEFINES += BITCOIN_QT_TEST
}

CODECFORTR = UTF-8

# for lrelease/lupdate
# also add new translations to src/qt/bitcoin.qrc under translations/
TRANSLATIONS = $$files(src/qt/locale/bitcoin_*.ts)

isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
isEmpty(QM_DIR):QM_DIR = $$PWD/src/qt/locale
# automatically build translations, so they can be included in resource file
TSQM.name = lrelease ${QMAKE_FILE_IN}
TSQM.input = TRANSLATIONS
TSQM.output = $$QM_DIR/${QMAKE_FILE_BASE}.qm
TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
TSQM.CONFIG = no_link
QMAKE_EXTRA_COMPILERS += TSQM

# "Other files" to show in Qt Creator
OTHER_FILES += \
    contrib/gitian-descriptors/* doc/*.rst doc/*.txt doc/README README.md res/bitcoin-qt.rc \
    share/setup.nsi

# platform specific defaults, if not overridden on command line
isEmpty(BOOST_THREAD_LIB_SUFFIX) {
    BOOST_THREAD_LIB_SUFFIX = $$BOOST_LIB_SUFFIX
}

isEmpty(BDB_LIB_SUFFIX) {
    macx:BDB_LIB_SUFFIX = -4.8
    win32:BDB_LIB_SUFFIX = -4.8
}

win32:DEFINES += WIN32
win32:RC_FILE = src/qt/res/bitcoin-qt.rc

win32:!contains(MINGW_THREAD_BUGFIX, 0) {
    # At least qmake's win32-g++-cross profile is missing the -lmingwthrd
    # thread-safety flag. GCC has -mthreads to enable this, but it doesn't
    # work with static linking. -lmingwthrd must come BEFORE -lmingw, so
    # it is prepended to QMAKE_LIBS_QT_ENTRY.
    # It can be turned off with MINGW_THREAD_BUGFIX=0, just in case it causes
    # any problems on some untested qmake profile now or in the future.
    DEFINES += _MT
    QMAKE_LIBS_QT_ENTRY = -lmingwthrd $$QMAKE_LIBS_QT_ENTRY
}

macx:HEADERS += src/qt/macdockiconhandler.h
macx:OBJECTIVE_SOURCES += src/qt/macdockiconhandler.mm
macx:LIBS += -framework Foundation -framework ApplicationServices -framework AppKit
macx:DEFINES += MAC_OSX MSG_NOSIGNAL=0
macx:ICON = src/qt/res/icons/bitcoin.icns
macx:QMAKE_CFLAGS_THREAD += -pthread
macx:QMAKE_CXXFLAGS_THREAD += -pthread
macx:QMAKE_INFO_PLIST = contrib/macdeploy/Info.plist

# libs
LIBS += -lssl -lcrypto -ldb_cxx$$BDB_LIB_SUFFIX -lleveldb -lmemenv -levent -lcryptopp -lz

# boost libs
LIBS += -lboost_system$$BOOST_LIB_SUFFIX -lboost_filesystem$$BOOST_LIB_SUFFIX -lboost_program_options$$BOOST_LIB_SUFFIX -lboost_thread$$BOOST_THREAD_LIB_SUFFIX -lboost_chrono$$BOOST_LIB_SUFFIX

# -lgdi32 has to happen after -lcrypto (see  #681)
win32:LIBS += -lws2_32 -lshlwapi -lmswsock -lole32 -loleaut32 -luuid -lgdi32
!win32:!macx:LIBS += -lrt -ldl -lgstreamer-0.10

contains(RELEASE, 1) {
    !win32:!macx {
        # Linux: turn dynamic linking back on for c/c++ runtime libraries
        LIBS += -Wl,-Bdynamic
    }
}

system($$QMAKE_LRELEASE -silent $$_PRO_FILE_)
