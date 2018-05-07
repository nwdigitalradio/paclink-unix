#!/bin/bash

PLU_VAR_DIR="/usr/local/var/wl2k"

USER=$(whoami)
GROUP="mail"

# set permissions for /usr/local/var/wl2k directory
# Check user & group of outbox directory
#
echo "=== test owner & group id of outbox directory"
if [ $(stat -c "%U" $PLU_VAR_DIR) != "$USER" ]  || [ $(stat -c "%G" $PLU_VAR_DIR) != "$GROUP" ] ; then
   echo "Outbox dir has wrong permissions: $(stat -c "%U" $PLU_VAR_DIR):$(stat -c "%G" $PLU_VAR_DIR)"
   sudo chown -R $USER:$GROUP $PLU_VAR_DIR
   echo "New permissions: $(stat -c "%U" $PLU_VAR_DIR):$(stat -c "%G" $PLU_VAR_DIR)"
else
   echo "permissions OK: $(stat -c "%U" $PLU_VAR_DIR):$(stat -c "%G" $PLU_VAR_DIR)"
fi

filecount=$(ls -1 ${PLU_VAR_DIR}/outbox | wc -l)
if [ -z $filecount ] ; then
  filecount=0
fi

if [ "$filecount" == 0 ] ; then
   echo "No files in outbox"
else
   echo "$filecount files in outbox"
fi
