Dfrotz Z-Machine Emulator
=========================

Download and compile the sources of frotz, this will build "dumb-frotz",
necessary as standard frotz is not able to produce plain text.

 git clone https://github.com/DavidGriffith/frotz

The above repo contains the documentation to build the sources.


OpenBSD Inetd
=============

Install and set up the openbsd `inetd` to listen for a simple telnet connection on a TCP port

`sudo apt install openbsd-inetd`

The `textgamesmenu` script included in the repo above has been tailored to work with two ports as BPQ automatically passes the connected callsign first before anything happens, so when someone connects over ax25 it automatically uses their callsign in prompts and such. For plain telnet connects (alternate port), the interactive menu will request the user to input their callsign first.


/etc/inetd.conf:


`games  stream  tcp     nowait  zorkuser  /usr/local/bin/textgamemenu client ax25`

`ipgames  stream  tcp     nowait  zorkuser /usr/local/bin/textgamemenu client ip`


Add `games` and `ipgames` to /etc/services, with a TCP port number.

E.g. 

` games  62000/tcp # LinBPQ Login for games`

` ipgames  61999/tcp # IP Login for games`


Modifying LinBPQ Configuration
==============================

Add a line to bpq32.cfg to assign an application that uses your BPQ telnet port 
(assuming you have one configured in bpq). 


There are two ways to do this in BPQ, the legacy method is to list the TCP ports on the `CMDPORT=` parameter, and then specify the offset number of that list in the `APPLICATION` clause.


Example CMDPORT parameters:


` CMDPORT=63000 23 63001 63002 62000 63003 63004 63005 63007`


Example APPLICATION definition:


`APPLICATION 7,GAMES,C 10 HOST 4 S`


Where `4` is the offset meaning port 63002 (CMDPORT begins at zero) in the above example.

Where `10` is the `PORTNUM` of your BPQ telnet port.

The ATTACH method *DOES NOT WORK*. So do NOT use the example below.

`APPLICATION 7,GAMES,ATTACH 10 127.0.0.1 63002 S` 


Make sure you restart inetd and test the port by using `telnet localhost 63002`



I may have to come back and edit this to clear up things but I hope it is at least readable! Looking forward to seeing more on packet. If you want to link with my node hit me up at `pe1rrr <.at.> amsat.org`



Notes:
======

Make sure you put the games in a directory that is owned by the user ID running the dfrotz emulator.
I run the dfrotz under a unique user ID 'zorkuser' that has it's shell set to `rbash` (restricted bash) with `rbash` set up according to the link below:


See https://veliovgroup.com/article/BmtWycSfZL37zXMZc/how-to-rbash

The dfrotz emulator can write save files anywhere on the filesystem where the userid has permission to do so, so be careful when implementing this before exposing it to the world and the potential dangers that come with that.

Example:
`drwxr-xr-x 3 myuserid      myuserid         4096 Jan 31 19:32 games`

`drwxr-xr-x 2 zorkuser zorkuser    4096 Jan  5 09:07 saves` 

As the game files are owned by `myuserid` rather than the 'zorkuser' ID that dfrotz is running, dfrotz won't be able to overwrite the game files with save files if someone malicious tries to do that. 

For simplicity, a symlink to the saves directory from the filesystem root is practical to use as it is always require to type out the full path to a save file in dfrotz. It can make it a lot shorter and easier to remember with a symlink. 

ln -s /path/to/actual/saves /saves
