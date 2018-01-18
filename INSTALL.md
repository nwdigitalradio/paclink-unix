# Installation notes for paclink-unix

**NOTE:** the code in the SourceForge repository is deprecated
* the active source code repository can be found on github at [this link](https://github.com/nwdigitalradio/paclink-unix):

```
https://github.com/nwdigitalradio/paclink-unix
```

To get a copy of the current repository:

```
git clone https://github.com/nwdigitalradio/paclink-unix
```

To update a local previously cloned repository:

```
cd <directory_of_previously_cloned_repository>
git pull
```

### Install links & notes

There is a build script [buildall.sh](https://github.com/nwdigitalradio/paclink-unix/blob/master/buildall.sh)
that automates the build process.
  * All build requirements need to be met before running this script.
  * Defaults to using postfix MTA, comment out _MTA=_ line to use sendmail MTA.
  * Use the following as user (NOT root) to rebuild everything:
```
./buildall.sh clean
```

* [useful install scripts](https://github.com/nwdigitalradio/n7nix/tree/master/plu/README.md)
* [wiki with comprehensive notes for basic install](http://bazaudi.com/plu/doku.php)
* [INSTALL](https://github.com/nwdigitalradio/paclink-unix/blob/master/INSTALL): original install file found in this repo
* [INSTALL.csv](https://github.com/nwdigitalradio/paclink-unix/blob/master/INSTALL.CVS) : original install file using SourceForge and [CVS](https://en.wikipedia.org/wiki/Concurrent_Versions_System) **<- deprecated**

### Example build on a Raspbery Pi

* As root get required files from Debian repositories.

```
apt-get update
apt-get install build-essential autoconf automake libtool
apt-get install postfix libdb-dev libglib2.0-0 zlib1g-dev libncurses5-dev libdb5.3-dev libgmime-2.6-dev
```
* Get a copy of the paclink-unix repository
```
git clone https://github.com/nwdigitalradio/paclink-unix
```

* Run the build script:
  * Errors are captured to _build_error.out_
  * Build output is captured to _build_log.out_

```
cd paclink-unix
./buildall.sh clean
Starting build from clean
=== building paclink-unix for MTA: postfix
This will take a few minutes, output is captured to /home/gunn/dev/paclink-unix/build_log.out
=== start from distclean
=== running autotools
=== running configure
=== making paclink-unix using 4 cores
=== installing paclink-unix
[sudo] password for gunn:
=== verifying paclink-unix install
Check for required installed files ... OK

2018 01 18 11:54:38 PST: paclink-unix build script FINISHED
```
