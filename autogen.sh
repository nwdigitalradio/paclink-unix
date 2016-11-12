#! /bin/sh

#  $Id$

#  paclink-unix client for the Winlink 2000 ham radio email system.
#
#  Copyright 2006 Nicholas S. Castellano <n2qz@arrl.net> and others,
#                 See the file AUTHORS for a list.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.

#  This script provides a convenient way for developers to rebuild after
#  modifying the autotools configuration.  Run this and then follow it up
#  with "make" to build.  You can pass any configure arguments to this script.
#  If you wish to inhibit the configure phase, pass the argument "--help" to
#  this script.

#  Normal users probably should not run this, follow the instructions
#  in INSTALL to build instead.

set -x
make distclean >/dev/null 2>&1
autoreconf
./configure $@
