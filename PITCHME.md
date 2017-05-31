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

* UNIX client interface to the Winlink 2000 ham radio email system.
* Allows using any movemail, or imap email client to compose/send/receive Winlink messages.
* Supports 3 transports, AX.25, telnet, serial port (pactor III using SCS-PTC-IIpro).

#VSLIDE

#### Use Case

* RPi on all the time running RMS Gateway, ssh access, use console based mail client
* RPi used for other things, use screen & keyboard probably with console based mail client
* RPi used for other things, use mobile device mail client & web control for paclink-unix

#VSLIDE

### Definitions

* MUA - Mail User Agent is an 'email client program', the part you see and use to send and receive email.
* MTA - Mail Transfer Agent is anything between MUAs.
  * MTAs generally do the routing and transferring of email between hosts, ISPs, etc.
* Mime - Multipurpose Internet Mail Extension
* SMTP - Simple Mail Transfer Protocol - Internet standard for sending messages to an email server.

#VSLIDE

#### How does it work?
###### Send a message

* Compose & address a Winlink message using your email client program
  * paclink-unix converts the regular email message into Winlink format.
  * Mail message files end up in outbox dir
* Use one of the paclink-unix transport methods to send the message.
* Winlink stores messages addressed to winlink.org & sends rest on to SMTP server.

#VSLIDE

#### How does it work? - con't
###### Receive a message

* Using one of paclink-unix transport methods contact a Winlink CMS
  * Winlink checks for any messages waiting to be delivered for your callsign.
* Paclink-unix gets the messages & converts them into standard email messages
  * paclink-unix then hands message to your MTA
  * MTA deposits converted mail messages into your regular email inbox.

#VSLIDE

#### How does it work? - con't
###### Processes

* Converts messages to/from GMime and Winlink formats
* Encodes/Decodes B2 Forwarding protocol
* Encodes/Decodes lzhuf compression
* Communicates through AX.25, TCP sockets or serial port

#HSLIDE

### Installation - overview

* Use either lite or regular compass image
  * paclink-unix does not require a window manager
* Use Install scripts then configure scripts
  * start here https://github.com/nwdigitalradio/n7nix
* Complete notes for a manual install on Dokuwiki
  * http://bazaudi.com/plu/doku.php?id=start

#VSLIDE

#### Installation - con't

* Basic installation includes these packages:
  * paclink-unix
  * postfix (MTA)
  * mutt (MUA)

#VSLIDE

#### Installation - con't

* Imap installation includes previous packages plus:
  * hostapd
  * dovecot
  * node.js

#VSLIDE

#### Configuration files

* paclink-unix config file lives here:
  *  _/usr/local/etc/wl2k.conf_
* postfix config files live in this directory:
  * _/etc/postfix_
* mutt config file lives here:
  * _/home/`<user>`/.muttrc_

#VSLIDE

#### Configuration Requirements

* Call sign
* Real Name
* Winlink password
  * **Note:** If you do not have a Winlink password leave it blank.

#VSLIDE

#### Configure with script
* Core (direwolf, ax25) must be config'd first

* As root run core config from _n7nix/config_ dir
```
cd n7nix/config
./app_config.sh core
```

* As root run paclink-unix config
  * then reboot

```
cd n7nix/config
./app_config.sh plu
shutdown -r now
```

#HSLIDE

#### Verify Install/config
##### Check core config

```
cd
cd bin
./ax25-status
```

#VSLIDE

#### Verify Install/config - con't
###### Create a Winlink mail message

* As user (pi) **NOT** root
* Use mutt to compose message
  * mutt spawns an editor, set editor in /home/`<user>`/.muttrc
* Address mail to yourself ie. **just your callsign**
  * mutt automatically appends winlink.org ie. n7nix@winlink.org
* After creating email in editor, exit editor & use mutt
  * 'y' to send
  * 'q' to Quit

#VSLIDE

#### Verify Install/config - cont'd
###### Send Winlink mail message

* Now check outbox directory
```
ls -salt /usr/local/var/wl2k/outbox/
```
* If there is a new file then send it via telnet
  * This will connect to a CMS

```
wl2ktelnet
```
* Check outbox directory again

#VSLIDE

#### Verify Install/config - cont'd
###### Receive Winlink mail

* Open another console window & monitor email log file
```
tail -f /var/log/mail.log
```
* Run _wl2ktelnet_
* Run Mutt again
  * Check that a new email message appears in your email client

#VSLIDE

#### Verify Install/config - cont'd
###### Locate an RMS Gateway or two

```
./gatewaylist.sh -m 35 -g cn85ax -s

```
```
./rmslist.sh 31 cn85ax

  Callsign        Frequency  Distance
  WA7VE-10        144960000    4
  W7CLA-10        144960000    9
  KD7IBA-10       144960000    14
  N7XRD-10        145010000    26
  W7CWY-10        145630000    31
```

#VSLIDE


#### Verify Install/config - cont'd
###### Set up some console windows

* Open up a packet spy in a console window (as root)
```
listen -at
```

* Open another console window & watch the email log file
```
tail -f /var/log/mail.log
```

#VSLIDE


#### Verify Install/config - cont'd
###### Using your radio

* As above create test message - look in outbox
* Connect to an RMS Gateway
```
wl2kax25 -c <RMS Gateway>
wl2kax25 -c WA7VE-10
```
* Check outbox directory again

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
