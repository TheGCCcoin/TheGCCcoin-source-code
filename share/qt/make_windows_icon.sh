#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/TheGCCcoin.ico

convert ../../src/qt/res/icons/TheGCCcoin-16.png ../../src/qt/res/icons/TheGCCcoin-32.png ../../src/qt/res/icons/TheGCCcoin-48.png ${ICON_DST}
