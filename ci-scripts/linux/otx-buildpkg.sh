#!/bin/bash
source /opt/qt59/bin/qt59-env.sh

echo ">>> Temporary install of OpenToonz"
export BUILDDIR=$(pwd)/toonz/build
cd $BUILDDIR
sudo make install

sudo ldconfig

echo ">>> Creating appDir"
rm -rf appdir
mkdir -p appdir/usr

echo ">>> Copy and configure OpenToonz installation in appDir"
cp -r /opt/opentoonz/* appdir/usr
cp appdir/usr/share/applications/*.desktop appdir
cp appdir/usr/share/icons/hicolor/*/apps/*.png appdir
mv appdir/usr/lib/opentoonz/* appdir/usr/lib
rmdir appdir/usr/lib/opentoonz

echo ">>> Creating OpenToonzPortable directory"
rm -rf OpenToonzPortable
mkdir OpenToonzPortable

echo ">>> Copying stuff to OpenToonzPortable/portablestuff"

mv appdir/usr/share/opentoonz/stuff OpenToonzPortable/portablestuff
chmod -R 777 OpenToonzPortable/portablestuff
rmdir appdir/usr/share/opentoonz

echo ">>> Creating OpenToonzPortable/OpenToonz.AppImage"

if [ ! -f linuxdeployqt*.AppImage ]
then
   wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
   chmod a+x linuxdeployqt*.AppImage
fi

export LD_LIBRARY_PATH=appdir/usr/lib/opentoonz
./linuxdeployqt*.AppImage appdir/usr/bin/OpenToonz -bundle-non-qt-libs -verbose=0 -always-overwrite \
   -executable=appdir/usr/bin/lzocompress \
   -executable=appdir/usr/bin/lzodecompress \
   -executable=appdir/usr/bin/tcleanup \
   -executable=appdir/usr/bin/tcomposer \
   -executable=appdir/usr/bin/tconverter \
   -executable=appdir/usr/bin/tfarmcontroller \
   -executable=appdir/usr/bin/tfarmserver 
./linuxdeployqt*.AppImage appdir/usr/bin/OpenToonz -appimage

mv OpenToonz*.AppImage OpenToonzPortable/OpenToonz.AppImage

echo ">>> Creating OpenToonz-OTX Linux package"

tar zcf OpenToonz-OTX-linux.tar.gz OpenToonzPortable
