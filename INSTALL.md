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
