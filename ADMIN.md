# MUSE Administration 

## Server Text Files
If you want to change anything in the 'help' or 'news' files, the format is self explanitory.

Make sure you execute a 'make' in the msgs directory after you change them.

A good idea would be to suggest using the 'gripe' command in connect.txt somewhere.

Various files that should be in the run/msgs directory:

- welcome.txt:  when someone connects to the port, but not to a char.
- connect.txt:  when someone connects to a character
- create.txt:   when someone creates a new character
- register.txt: when someone tries to create a char, but WCREAT is in force.

<sup>Note that these are just blank if they don't exist. Server won't complain about it or anything.</sup>

## Admin Commands
Here are some commands that would probably only be useful to someone running a MUSE.

### @allquota <quota>
changes everyone's quota to <quota>.

### @broadcast <message>
broascast <message> through no_wall flags. it would be best
to only use this for inportant messages, such as the muse going
down.

### @boot <player>
disconnect <player> from the game. if there are more than one
<player>s connected, disconnect the one that was connected
most recently.

### @chownall <player>=<newplayer>
change the ownershi pof all <player>'s objects to <newplayer>.

### @ctrace
trace all concentrator connections. there is no concentrator
client right now, so this is sorta useless.

### @class <player>=<new class>
change <player>'s class to <new class>. see doc/CLASSES.

### @dbck
do various things to make sure the database is all in
order.

### @dbtop [<catagories>]
show highest users of space; catagories are shown with
just plain '@dbtop'.

### @dump
checkpoint the database onto disk. (if you try to do this
at the same time that muse is doing an automatic dump,
it may give an error (db/mdb.#xxx#: no such file or
directory). it will have saved the database anyways,
though.)

### @empower <player>=<power>:<class>
give <player> the power of <power> to act on <class>.
<power> can be one of the powers defined in powers.h
<class> can be one of:
yes: always
yeseq: for equal and lower classes
yeslt: for lower classes
no: never; take away the power.

### @mailhuh
Mail out huh logs; this isn't currently tested very much.

### @newpassword <player>=<password>
change <player>'s password to <password>. if you're thinking
of using this to change your own password, don't. use
@password instead. passwords are plaintext after @newpassword.

### @nuke <player>
destroy <player>. doesn't work if they own stuff; need to
@wipeout first.

### @pbreak
show various statistics on how many people are in which class.

### @pcreate <name>=<password>
only effective if you have WCREAT defined. creates a new
character, name <name>, password <password>.

### @poor <value>
make everyone's credits change to <value>. this isn't very
good to do.

### @purge
same as @dbck, but doesn't do quite as much stuff.

### @reboot
Reboot the muse. easier than @shutdown and then going
and restarting the wd.

### @shutdown
shut down the muse. probably would be a good thing to
@broascast telling people about this first.

### @wipeout <player> type=<type>
wipe out all possessions of <player> of type <type>.
<type> can be:
all: wipeout all <player>'s possessions
rooms: wipeout all rooms
exits: wipeout all exits
objects: wileout all objects
