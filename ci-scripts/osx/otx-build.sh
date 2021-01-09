#!/bin/bash
pushd thirdparty/tiff-4.0.3
./configure --disable-lzma --without-x && make
popd

cd toonz

if [ ! -d build ]
then
   mkdir build
fi
cd build

QTVERSION=`ls /usr/local/Cellar/qt`
echo "QT Version detected: $QTVERSION"

export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/opt/jpeg-turbo/lib/pkgconfig"
cmake ../sources \
      -DQT_PATH=/usr/local/opt/qt/lib/ \
      -DTIFF_INCLUDE_DIR=../../thirdparty/tiff-4.0.3/libtiff/ \
      -DSUPERLU_INCLUDE_DIR=../../thirdparty/superlu/SuperLU_4.1/include/

make -j7 # runs 7 jobs in parallel
