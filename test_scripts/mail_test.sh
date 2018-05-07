#!/bin/bash
#
# Set these variables"
#   sendto="some_callsign"
#   cc="some_callsign"
# Could also use a fully qualified email address
#   sendto="bob@realmail.com"
# Only required to set "sendto="

sendto=
ccto=

INDEX_FILENAME="indexfile.txt"
MSG_FILENAME="testmsg.txt"

scriptname="`basename $0`"
MUTT="/usr/bin/mutt"
# Set boolean for this script generated email body
bgenbody="false"

# has the mutt email program been installed
type -P $MUTT &>/dev/null
if [ $? -ne 0 ] ; then
   # Get here if mutt program NOT installed.
   echo "$scriptname: mutt not installed."
   exit 1
fi

if [ -z "$sendto" ] ; then
   echo "Need to provide a sendto callsign or email address."
   echo "Set sendto variable at top of shell script."
   exit 1
fi

# if index file doesn't exist create it & set contents to a zero
if [ ! -e "$INDEX_FILENAME" ] ; then
  echo "INFO: file $INDEX_FILENAME DOES NOT exist"
  echo "0" > $INDEX_FILENAME
fi

# Load the test index
testindex=$(cat $INDEX_FILENAME)

# Look for a file to make email body
#   - if it doesn't exist make something up
if [ ! -e "$MSG_FILENAME" ] ;then
{
  echo "Sent on: $(date "+%m/%d %T %Z")"
  echo "/$(whoami)"
  bgenbody="true"

} > $MSG_FILENAME
fi

# get the test file size
FILESIZE=$(stat -c%s "$MSG_FILENAME")
echo "Test $(date "+%m/%d/%y") #$testindex, File size of $MSG_FILENAME = $FILESIZE"

subject="//WL2K $(hostname) plu $(date "+%m/%d/%y"), size: $FILESIZE, #$testindex"

echo "$subject"
echo "plu outbox owner: $(stat -c '%U' /usr/local/var/wl2k) $(stat -c '%G' /usr/local/var/wl2k/outbox)"

# Test if Cc: is ued
if [[ -z "$ccto" ]] ; then
   $MUTT -s "$subject" $sendto < $MSG_FILENAME
else
   $MUTT -s "$subject" -c $ccto $sendto < $MSG_FILENAME
fi

# increment the index
let testindex=$testindex+1

# Write new index back out to index file
echo "$testindex" > $INDEX_FILENAME

# If this script generated the mail body delete the temporary file.
if  [ "$bgenbody" = "true" ] ; then
   rm "$MSG_FILENAME"
fi
echo "email generation finished."
exit 0
