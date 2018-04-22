#!/bin/bash
#
# Build & install paclink-unix
#  - any command line args causes build from clean
#
#DEBUG=1
#MAKE_NO_INSTALL="true"
MAKE_NO_INSTALL=

scriptname="`basename $0`"

# MTA sets command line option for sendmail
#   Defaults to sendmail flags -ba
#   Setting MTA to --enable-postfix sets sendmail flags to -bm
# If not using postfix as the mta then comment next line
MTA="--enable-postfix"

clean_arg="false"
REQUIRED_BUILD_FILES="missing test-driver README"

# ===== function check_files

function check_files() {
for filename in $REQUIRED_BUILD_FILES ; do
   if [ ! -f "$filename" ] ; then
      echo "$scriptname: File not found: $filename, build aborted"
      exit 1
   fi
done
}

# ===== main
# Don't need to be root
if [[ $EUID == 0 ]] ; then
   echo
   echo "Don't need to be root"
   echo
   exit 1
fi

# Check if there are any args on command line
if (( $# != 0 )) ; then

   if [ "$1" == "clean" ] ; then
      echo "Starting build from clean"
      clean_arg="true"
   fi

else
   echo "Starting build without clean"
fi


# Save current directory
CUR_DIR=$(pwd)

MTA_STRING="postfix"
if [ -z "$MTA" ] ; then
   MTA_STRING="sendmail"
fi
echo "=== building paclink-unix for MTA: $MTA_STRING"
echo "This will take a few minutes, output is captured to $(pwd)/build_log.out"

cp README.md README

echo "$(date "+%Y %m %d %T %Z"): paclink-unix build script START" > build_error.out
echo "$(date "+%Y %m %d %T %Z"): paclink-unix build script START" > build_log.out

if [ "$clean_arg" == true ] ; then
   echo "=== start from distclean"
   make distclean >> build_error.out 2>&1
fi

echo "=== running autotools"
aclocal >> build_log.out 2>> build_error.out
if [ "$?" -ne 0 ] ; then echo "build failed at aclocal"; exit 1; fi

autoheader >> build_log.out 2>> build_error.out
if [ "$?" -ne 0 ] ; then echo "build failed at autoheader"; exit 1; fi

automake --add-missing >> build_log.out 2>> build_error.out
if [ "$?" -ne 0 ] ; then echo "build failed at automake"; exit 1; fi

autoreconf >> build_log.out 2>> build_error.out

echo "=== running configure"
./configure $MTA >> build_log.out 2>> build_error.out
if [ "$?" -ne 0 ] ; then echo "build failed at configure"; exit 1; fi

check_files

num_cores=$(nproc --all)
echo "=== making paclink-unix using $num_cores cores"
make -j$num_cores >> build_log.out 2>> build_error.out
if [ "$?" -ne 0 ] ; then echo "build failed at make"; exit 1; fi

if [ "$MAKE_NO_INSTALL" != "true" ] ; then
   echo "=== installing paclink-unix"
   sudo make install >> build_log.out 2>> build_error.out
   if [ "$?" -ne 0 ] ; then echo "build failed at make install"; exit 1; fi

   if [ ! -z "$DEBUG" ] ; then
      echo "=== test for required build files 'missing' & 'test-driver'"
      ls -salt $CUR_DIR/missing $CUR_DIR/test-driver
   fi

   echo "=== verifying paclink-unix install"
   REQUIRED_PRGMS="wl2ktelnet wl2kserial wl2kax25 wl2kax25d"

   install_status="OK"
   for prog_name in `echo ${REQUIRED_PRGMS}` ; do
      type -P $prog_name &>/dev/null
      if [ $? -ne 0 ] ; then
         echo "$scriptname: paclink-unix not installed properly"
         echo "$scriptname: Need to Install $prog_name program"
         install_status="FAIL"
      fi
   done
   echo "Check for required installed files ... $install_status"

   if [[ $EUID != 0 ]] ; then
      user=$(whoami)
      sudo chown -R $user:mail /usr/local/var/wl2k
   fi
fi  # MAKE_NO_INSTALL == true

echo
echo "$(date "+%Y %m %d %T %Z"): paclink-unix build script FINISHED"
echo "$(date "+%Y %m %d %T %Z"): paclink-unix build script FINISHED" >> build_log.out
