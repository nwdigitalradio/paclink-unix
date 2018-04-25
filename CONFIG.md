# Configuration notes for paclink-unix

### Client Mode

This is the normal usage case for paclink-unix where you will connect
to an RMS Gateway to send & receive Winlink messages.

The configuration file for paclink-unix is _/usr/local/etc/wl2k.conf_

At a minimum you need to set the following in the _wl2k.conf_ file
```
mycall=
email=
wl2k-password=
ax25port=
```

* _mycall_ should be your callsign with no SSID
* _email_ is your Internet email address.
  * If you set up dovecot it will be your local email address ie. <user_name>@localhost
  * **DO NOT** use your winlink email address.
* _wl2k-pasword_ is the password you received [from Winlink](https://www.winlink.org/tags/password)
* _ax25port_ is the portname of your ax25 device found in _/etc/ax25/axports_

The default values for the rest of the configuration variables will be
fine for most cases.

### Daemon mode

Daemon mode allows other Winlink clients to connect to your station
for peer to peer delivery of Winlink messages.

If you are using paclink_unix for peer to peer messaging you also need
to set the following in _wl2k.conf_:
```
gridsquare=
welcomemsg=
```

For example:
```
# Gridsquare locator
gridsquare=CN88nl

# Welcome message
welcomemsg="Welcome to N7NIX's Winlink mailbox\r"
```
* paclink-unix listen's for it's call sign using ax25d
  * Need to have an entry in _/etc/ax25/ax25d.conf_ file

```
[<CALLSIGN> VIA <PORTNAME>]
NOCALL   * * * * * *  L
default  * * * * * *  - <USER> /usr/local/bin/wl2kax25d wl2kax25d -c %U -a %d
```
* _CALLSIGN_ should be your callsign with no SSID
* _PORTNAME_ is the portname of your ax25 device found in _/etc/ax25/axports_
* _USER_ is your normal login user name (NOT ROOT)

For example:
```
[N7NIX VIA udr0]
NOCALL   * * * * * *  L
default  * * * * * *  - gunn /usr/local/bin/wl2kax25d wl2kax25d -c %U -a %d
```
