#!/bin/bash
source /opt/qt59/bin/qt59-env.sh

echo ">>> Temporary install of OpenToonz"
export BUILDDIR=$(pwd)/toonz/build
cd $BUILDDIR
sudo make install

sudo ldconfig

echo ">>> Creating appDir"
if [ -d appdir ]
then
   rm -rf appdir
fi
mkdir -p appdir/usr

echo ">>> Copy and configure OpenToonz installation in appDir"
cp -r /opt/opentoonz/* appdir/usr
cp appdir/usr/share/applications/*.desktop appdir
cp appdir/usr/share/icons/hicolor/*/apps/*.png appdir

echo ">>> Creating OpenToonzPortable directory"
if [ -d OpenToonzPortable ]
then
   rm -rf OpenToonzPortable
fi

echo ">>> Creating OpenToonzPortable/OpenToonz.AppImage"

if [ ! -f linuxdeployqt*.AppImage ]
then
   wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
   chmod a+x linuxdeployqt*.AppImage
fi

if [ ! -f appimagetool*.AppImage ]
then
   wget -c "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
   chmod a+x appimagetool*.AppImage
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

rm appdir/AppRun

cat << EOF >> appdir/AppRun
#!/bin/bash

SCRIPT_FILE=\`realpath "\$0"\`
SCRIPT_DIR=\`dirname "\$SCRIPT_FILE"\`
export LD_LIBRARY_PATH=\${SCRIPT_DIR}/usr/lib/:\${LD_LIBRARY_PATH}
\${SCRIPT_DIR}/usr/bin/opentoonz "\$@"
EOF

chmod +x appdir/AppRun
./appimagetool*.AppImage appdir
