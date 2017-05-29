## paclink-unix
##### Basil Gunn N7NIX,  June 2017
###### https://gitpitch.com/nwdigitalradio/paclink-unix/

#HSLIDE

### History
* First entry in SourceForge repository Nov 10, 2005

#### Authors
* Nicholas S. Castellano N2QZ
* Dana Borgman KA1WPM
* Basil Gunn N7NIX

#### Patches
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
* SMTP - Simple Mail Transfer Protocol - Internet standard for sending messages to a mail server.

#VSLIDE

#### How does it work?
###### Send a message

* Compose & address a Winlink message using your mail program
  * paclink-unix converts the regular email message into Winlink format.
  * Mail message files end up in /usr/local/var/wl2k/outbox
* Use one of the paclink-unix transport methods to send the message.
* Winlink stores messages addressed to winlink.org & sends the rest on to an SMTP server.

#VSLIDE

#### How does it work? - con't
###### Receive a message

* Using one of paclink-unix transport methods contact a Winlink CMS
  * Winlink checks for any messages waiting to be delivered for your callsign.
* Paclink-unix gets the messages & converts them into standard email messages
  * paclink-unix then hands message to you MTA
  * MTA deposits converted mail messages into your regular email inbox.

#VSLIDE

#### How does it work? - con't
###### Processes

* Converts messages to/from GMime and Winlink formats
* Encodes/Decodes B2F protocol
* Encodes/Decodes lzhuf compression
* Communicates through AX.25, TCP sockets or serial port

#VSLIDE

Diagram or 2 here

#HSLIDE

### Installation - overview

* Use either lite or regular compass image
  * paclink-unix does not require a window manager
* Use Install scripts then configure scripts
  * start here https://github.com/nwdigitalradio/n7nix
* Complete notes for a manual install at Dokuwiki
  * http://bazaudi.com/plu/doku.php?id=start

#VSLIDE

### Installation - con't

* Basic installation includes these packages:
  * paclink-unix
  * postfix (MTA)
  * mutt (MUA)

#VSLIDE

### Installation - con't

* Imap installation includes previous packages plus:
  * hostapd
  * dovecot
  * node.js

#VSLIDE

#### Configuration Requirements

* Call sign
* Real Name
* Winlink password
  * **Note:** If you do not have a Winlink password leave it blank.

#VSLIDE

#### Configuration files

* paclink-unix config file lives here:
  *  _/usr/local/etc/wl2k.conf_
* postfix config files live here:
  * /etc/postfix
* mutt config file lives here:
  * /home/`<user>`/.muttrc
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
* Check outbox directory
```
ls -salt /usr/local/var/wl2k/outbox/
```
* If there is a new file, send it via telnet
```
wl2ktelnet
```

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
