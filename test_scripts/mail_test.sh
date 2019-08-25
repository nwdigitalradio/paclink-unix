#!/bin/bash
#
# Set these variables"
#   sendto="some_callsign"
#   cc="some_callsign"
# Could also use a fully qualified email address
#   sendto="bob@realmail.com"
# Only required to set "sendto="
DEBUG=
sendto=
ccto=

INDEX_FILENAME="indexfile.txt"
MSG_FILENAME="testmsg.txt"

scriptname="`basename $0`"
MUTT="/usr/bin/mutt"
# Set boolean for this script generated email body
bgenbody=false

## ============ functions ============

function dbgecho { if [ ! -z "$DEBUG" ] ; then echo "$*"; fi }

#
# Display program help info
#
usage () {
	(
	echo "Usage: $0 <ham_callsign> <size_body_of_message>"
	echo " exiting ..."
	) 1>&2
	exit 2
}

# ===== main

MSGSIZE=0


# Check for any command line arguments
# Arg 1: sendto callsign
# Arg 2: message size

if (( $# > 0 )) ; then
    CALLSIGN="$1"
    # Validate callsign
    sizecallstr=${#CALLSIGN}

    if (( sizecallstr > 6 )) || ((sizecallstr < 3 )) ; then
        echo "Invalid call sign: $CALLSIGN, length = $sizecallstr"
        exit 1
    fi
    sendto=$CALLSIGN
fi


# check for a second command line argument - message size

if (( $# >= 2 )) ; then
    MSGSIZE="$2"

    # Verify MSGSIZE is a number
    if ! [[ $MSGSIZE =~ ^[0-9]+$ ]] ; then
        echo
        echo "INVALID message size : $MSGSIZE, needs to be a number"
        MSGSIZE=0
    fi
fi

dbgecho "MSGSIZE specified: $MSGSIZE"
if (( $MSGSIZE > 0 )) ; then
    # Message size is a vaild number & greater than 0
    dbgecho "MSGSIZE is greater than 0"
    < /dev/urandom tr -dc "\n [:alnum:]" | head -c${MSGSIZE} > $MSG_FILENAME
    bgenbody=true
else
    echo "MSGSIZE not greater than 0"
    echo "Removing existing $MSG_FILENAME"
    # git rid of existing MSG_FILENAME
    if [ -e "$MSG_FILENAME" ] ;then
        rm "$MSG_FILENAME"
    fi
fi

if [ ! -z "$DEBUG" ] ; then
    echo
    echo "DEBUG: dumping $MSG_FILENAME"
    cat $MSG_FILENAME
    echo
fi

# Look for a file to make email body
#   - if it doesn't exist make something up
if [ ! -e "$MSG_FILENAME" ] ;then
{
  echo "Sent on: $(date "+%m/%d %T %Z")"
  echo "/$(whoami)"
  bgenbody=true

} > $MSG_FILENAME
fi

# get the test file size
FILESIZE=$(stat -c%s "$MSG_FILENAME")

# has the mutt email program been installed
type -P $MUTT &>/dev/null
if [ $? -ne 0 ] ; then
   # Get here if mutt program NOT installed.
   echo "$scriptname: mutt not installed."
   exit 1
fi

if [ -z "$sendto" ] ; then
   echo "Need to provide a sendto callsign or email address."
   usage
   exit 1
fi

# if index file doesn't exist create it & set contents to a zero
if [ ! -e "$INDEX_FILENAME" ] ; then
  echo "INFO: file $INDEX_FILENAME DOES NOT exist, creating."
  echo "0" > $INDEX_FILENAME
fi

# Load the test index
testindex=$(cat $INDEX_FILENAME)

echo "Test $(date "+%m/%d/%y") #$testindex, File size of $MSG_FILENAME = $FILESIZE bytes"

subject="//WL2K $(hostname) plu $(date "+%m/%d/%y"), size: $FILESIZE, #$testindex"

echo "$subject"
echo "plu outbox owner: $(stat -c '%U' /usr/local/var/wl2k) : $(stat -c '%G' /usr/local/var/wl2k/outbox)"

# Test if Cc: is used
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
if  $bgenbody ; then
   rm "$MSG_FILENAME"
fi
echo "email generation finished."
exit 0
