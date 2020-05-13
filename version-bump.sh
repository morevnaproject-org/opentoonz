#!/bin/sh

set -e

DIRNAME=`dirname "$0"`
cd $DIRNAME
DIRNAME=`pwd`

if [ -z "$2" ]; then
	echo "USAGE: $0 VERSION REVISION"
	echo "Example: $0 1.4 1"
	exit 1
fi

VERSION_NEW="$1"
RELEASE_NEW="$2"

sed -i "s|version: .*\.{build}|version: ${VERSION_NEW}.${RELEASE_NEW}.{build}|" appveyor.yml
sed -i "s|.*const float applicationVersion.*|  const float applicationVersion  = ${VERSION_NEW};|" toonz/sources/include/tversion.h
sed -i "s|.*const float applicationRevision.*|  const float applicationRevision  = ${RELEASE_NEW};|" toonz/sources/include/tversion.h


git add \
	appveyor.yml \
	toonz/sources/include/tversion.h \
	# end

git commit -m "Bump version to ${VERSION_NEW}.${RELEASE_NEW}"
