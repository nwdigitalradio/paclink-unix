## paclink-unix
##### Basil Gunn N7NIX,  June 2017
###### https://gitpitch.com/nwdigitalradio/paclink-unix/

#HSLIDE

### History
* First entry in SourceForge repository Nov 10, 2005

### Authors
* Nicholas S. Castellano N2QZ
* Dana Borgman KA1WPM
* Basil Gunn N7NIX

### Patches
* John Ronan EI7IG
* Brian Smith WB4ES

#HSLIDE

### What is paclink-unix?

* UNIX client interface to the Winlink 2000 ham radio e-mail system.
* Allows using any movemail, or imap e-mail client to compose/send/receive winlink messages.
* Supports 3 transports, AX.25, telnet, serial port (pactor III using SCS-PTC-IIpro)

#VSLIDE

### Definitions

* MUA - Mail User Agent is a 'mail client program', the part you see and use to send and receive mail.
* MTA - Mail Transfer Agent is anything between MUAs. MTAs generally do the routing and transferring of mail between hosts, ISPs, etc.
* Mime - Multipurpose Internet Mail Extension

#VSLIDE

### How does it work?
* Converts messages to/from GMime and Winlink formats
* Encodes/Decodes B2F protocol
* Encodes/Decodes lzhuf compression
* Communicates through AX.25 & TCP sockets

#VSLIDE

Diagram here

#HSLIDE

### Installation - overview

* Use Install scripts then configure scripts
  * start here https://github.com/nwdigitalradio/n7nix
* Complete notes for a manual install at Dokuwiki
  * http://bazaudi.com/plu/doku.php?id=start

#VSLIDE

### Installation - con't

postfix

#VSLIDE

#### Configuration Requirements

* Call sign
* Real Name
* Winlink password
  * **Note:** If you do not have a Winlink password leave it blank.

#VSLIDE

#### Configuration file in /usr/local/etc/ directory
* wl2k.conf

* As root run app_config.sh plu
```
cd n7nix/config
./app_config plu
```
#HSLIDE

#### Verify Install/config
###### Send mail
* Use mutt
* Address mail to yourself ie. just your callsign
* After creating email hit 'y' to send, then 'q' to Quit.
* Check outbox directory again
```
ls -salt /usr/local/var/wl2k/outbox/
```
* If there is a new file, send it via telnet
```
wl2ktelnet
```

#### Verify Install/config
###### Send mail



#HSLIDE

### Where to get help

* man pages
* Forums
* paclink-unix dokuwiki
* Google Fu

#VSLIDE

#### man pages

* wl2k.conf
* wl2kax25
* wl2kax25d - under development
* wl2kserial
* wl2ktelnet

#VSLIDE

#### Forum

* https://groups.yahoo.com/neo/groups/paclink-unix/info

#### Installation Guide

* http://bazaudi.com/plu/

#HSLIDE

#### That's it

* This presentation is a GitPitch & can be found here:

###### https://gitpitch.com/nwdigitalradio/paclink-unix/
