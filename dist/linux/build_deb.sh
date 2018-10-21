#!/bin/bash

dir=`pwd`

if [ $# -ne 1 ]
then
    echo "1 arg needed to build deb, ./build.sh <version>"
    exit
fi

ver=$1

rm -rf launchy-$ver*
rm launchy_$ver*
svn export ../ launchy-$ver
tar cfz launchy-$ver.tar.gz launchy-$ver/
cd launchy-$ver
dh_make -e karlinjf@sourceforge.net -f ../launchy-$ver.tar.gz -c gpl --single
cp ../debian/* debian/
qmake -r Launchy.pro
INSTALL_ROOT=$DESTDIR dpkg-buildpackage -rfakeroot

cd ..

#mkdir repository
#mkdir repository/binary
#mkdir repository/source
#cp *.deb repository/binary/
#cp *.orig.* *.diff.gz *.dsc  repository/source/
#cd repository
#dpkg-scanpackages binary /dev/null | gzip -9c > binary/Packages.gz
#dpkg-scansources source /dev/null | gzip -9c > source/Sources.gz