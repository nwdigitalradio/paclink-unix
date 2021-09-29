#!/bin/bash

PLU_VAR_DIR="/usr/local/var/wl2k"
MID_DB_FILE="$PLU_VAR_DIR/mid.db"

USER=$(whoami)
GROUP="mail"

if [[ $EUID == 0 ]] ; then
   echo
   echo "You are running this script as root ... don't do that."
   exit 1
fi

# set permissions for /usr/local/var/wl2k directory
# Check user & group of outbox directory
#
echo "=== test owner & group id of outbox directory"
if [ $(stat -c "%U" $PLU_VAR_DIR) != "$USER" ]  || [ $(stat -c "%G" $PLU_VAR_DIR) != "$GROUP" ] ; then
   echo "Outbox dir has wrong permissions: $(stat -c "%U" $PLU_VAR_DIR):$(stat -c "%G" $PLU_VAR_DIR)"
   sudo chown -R $USER:$GROUP $PLU_VAR_DIR
   echo "New permissions: $(stat -c "%U" $PLU_VAR_DIR):$(stat -c "%G" $PLU_VAR_DIR)"
else
   echo "permissions OK on directory $PLU_VAR_DIR: $(stat -c "%U" $PLU_VAR_DIR):$(stat -c "%G" $PLU_VAR_DIR)"
fi

echo "=== test owner & group id of message id database"
if [ $(stat -c "%U" $MID_DB_FILE) != "$USER" ]  || [ $(stat -c "%G" $MID_DB_FILE) != "$GROUP" ] ; then
   echo "MID database has wrong permissions: $(stat -c "%U" $MID_DB_FILE):$(stat -c "%G" $MID_DB_FILE)"
   sudo chown $USER:$GROUP $MID_DB_FILE
   echo "New permissions: $(stat -c "%U" $MID_DB_FILE):$(stat -c "%G" $MID_DB_FILE)"
else
   echo "permissions OK on file $MID_DB_FILE: $(stat -c "%U" $MID_DB_FILE):$(stat -c "%G" $MID_DB_FILE)"
fi

filecount=$(ls -1 ${PLU_VAR_DIR}/outbox | wc -l)
if [ -z $filecount ] ; then
  filecount=0
fi

if [ "$filecount" == 0 ] ; then
   echo "No files in outbox"
else
   for filename in ${PLU_VAR_DIR}/outbox/* ; do
       [ -e "$filename" ] || continue
       filesize=$(stat -c %s $filename)
       bname="$(basename $filename)"
#      echo "debug: filename: $filename, base: $bname"
       echo -e "$bname\t$filesize"
   done
   echo "$filecount file(s) in outbox"
fi
