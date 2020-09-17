#!/bin/bash
export QTDIR=/usr/local/opt/qt
export TOONZDIR=toonz/build/toonz

echo ">>> Copying stuff to $TOONZDIR/OpenToonz.app/portablestuff"
cp -R stuff $TOONZDIR/OpenToonz.app/portablestuff

echo ">>> Configuring OpenToonz.app for deployment"

$QTDIR/bin/macdeployqt $TOONZDIR/OpenToonz.app -verbose=0 -always-overwrite \
   -executable=$TOONZDIR/OpenToonz.app/Contents/MacOS/lzocompress \
   -executable=$TOONZDIR/OpenToonz.app/Contents/MacOS/lzodecompress \
   -executable=$TOONZDIR/OpenToonz.app/Contents/MacOS/tcleanup \
   -executable=$TOONZDIR/OpenToonz.app/Contents/MacOS/tcomposer \
   -executable=$TOONZDIR/OpenToonz.app/Contents/MacOS/tconverter \
   -executable=$TOONZDIR/OpenToonz.app/Contents/MacOS/tfarmcontroller \
   -executable=$TOONZDIR/OpenToonz.app/Contents/MacOS/tfarmserver 

echo ">>> Correcting library paths"
for X in `find $TOONZDIR/OpenToonz.app/Contents -type f -name *.dylib -exec otool -l {} \; | grep -e "^toonz" -e"name \/usr\/local" | sed -e"s/://" -e"s/ (.*$//" -e"s/^ *name //"`
do
   Z=`echo $X | cut -c 1-1`
   if [ "$Z" != "/" ]
   then
      LIBFILE=$X
   else
      Y=`basename $X`
      W=`basename $LIBFILE`
      if [ -f $TOONZDIR/OpenToonz.app/Contents/Frameworks/$Y -a "$Y" != "$W" ]
      then
         echo "Fixing $X in $LIBFILE"
         install_name_tool -change $X @executable_path/../Frameworks/$Y $LIBFILE
      fi
   fi
done
   
echo ">>> Creating OpenToonz-OTX-osx.dmg"

$QTDIR/bin/macdeployqt $TOONZDIR/OpenToonz.app -dmg -verbose=0

mv $TOONZDIR/OpenToonz.dmg $TOONZDIR/../OpenToonz-OTX-osx.dmg
