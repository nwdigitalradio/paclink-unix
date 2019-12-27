# Welcome to the paclink-unix distribution.

[![GitPitch](https://gitpitch.com/assets/badge.svg)](https://gitpitch.com/nwdigitalradio/paclink-unix/master)

paclink-unix is a UNIX/Linux client for the Winlink 2000 ham radio email
system. It allows the use of email clients to compose & read Winlink
messages. Currently mutt, claws-mail, rainloop & thunderbird are
actively supported but any mail client that uses IMAP should work.

paclink-unix was originally written by Nicholas S. Castellan N2QZ

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
[GNU General Public License](http://www.gnu.org/licenses/gpl-2.0.html) for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

See the COPYING file for the complete license or click on [this
link.](http://www.gnu.org/licenses/gpl-2.0.html)

Installation notes can be found at [this
link](https://github.com/nwdigitalradio/paclink-unix/blob/master/INSTALL.md)

### Forum & code repository
* Both yahoo groups & the SourceForge repository are deprecated please
do not use them. Instead use:
  * __Code Repository__:
  https://github.com/nwdigitalradio/paclink-unix
  * __Forum__: https://groups.io/g/paclink-unix/topics

### Installation tips
* To get a copy of the repository using git:

```
git clone https://github.com/nwdigitalradio/paclink-unix
```
* or using wget to create a zipped tarball
```
# To create a tar zipped file, rmsgw.tgz
wget -O paclink-unix.tgz https://github.com/nwdigitalradio/paclink-unix/tarball/master

# To create a directory with source files from the zipped tarball
wget -O - https://github.com/nwdigitalradio/rmsgw/tarball/master | tar zxf
```

* There are some detailed installation notes [at this
link](http://bazaudi.com/plu/doku.php)

* paclink-unix is built using
[autotools](https://www.gnu.org/software/automake/manual/html_node/Autotools-Introduction.html)
  * To build paclink-unix you can use the [_buildall.sh_
  script](https://github.com/nwdigitalradio/paclink-unix/blob/master/buildall.sh). See
  [Installation
  Notes](https://github.com/nwdigitalradio/paclink-unix/blob/master/INSTALL.md)

### Recent news

* The entire change log can be [viewd
here](https://github.com/nwdigitalradio/paclink-unix/blob/master/ChangeLog)

##### 12/26/2019 version 0.9
* The QTC line is not output anymore due to linBPQ32 6.0.19.1 crashing when it tries to parse it.
  *  To be closer to what Winlink Express looks like that line now looks like the following.
  * This means that _gridsquare=_ needs to be set in the configuration file.
```
; VE7SPR-10 DE N7NIX (CN88nl)
```

##### 5/9/2019 version 0.8
* Fixed upper casing password in hash_input
  * Multi case passwords now work.

##### 4/27/2018 version 0.7
* Peer-to-peer working with Airmail & Winkink Express


### Other files you might want to peruse:

| File name     |  Description  |
| --------------|---------------|
| **AUTHORS**	| Who's involved |
| **ChangeLog**	| Detailed list of changes |
| **NEWS**	| Summary of feature changes and fixes |
| **COPYING**   | GPL version 2 |

