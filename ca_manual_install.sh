#!/bin/bash

#
# NOTE: This script allows non-root users to install part of CodeAnalyst
#       which does not allow root permission.
#	(Requested for Open64 QA team)
#

if test $# -eq 0 ; then
	echo "Usage: ./ca_manual_install.sh <prefix>"
	exit 1
fi

PREFIX=$1

make -C doc            install INSTALL="install -p"
make -C src/ca/libs    install INSTALL="install -p"
make -C src/ca/gui     install INSTALL="install -p"
make -C src/ca/diffgui install INSTALL="install -p"
make -C src/ca/utils   install INSTALL="install -p"
make -C src/ca/scripts install INSTALL="install -p"
make -C src/oprofile/daemon  install INSTALL="install -p"
make -C src/oprofile/utils   install INSTALL="install -p"
make -C src/oprofile/pp      install INSTALL="install -p"
make -C src/oprofile/events  install INSTALL="install -p"
install -pDm 755 careport.sh ${PREFIX}/bin/careport.sh
install -pDm 755 src/dwarf-20090217/libdwarf/libdwarf.so $PREFIX/lib/libdwarf.so

# This requires root to run. (One time set up)
#pushd src/ca/
#sh scripts/Setup.sh $PREFIX
#popd
