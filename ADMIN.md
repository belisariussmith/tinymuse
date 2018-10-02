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

### God / Wizard Accounts

The database has two player accounts by default, God and Wizard. Their passwords are mike &amp; fooblee, respectively.

## Admin Commands
Here are some commands that would probably only be useful to someone running a MUSE.

### @allquota &lt;quota&gt;
changes everyone's quota to &lt;quota&gt;.

### @broadcast &lt;message&gt;
broascast &lt;message&gt; through no_wall flags. it would be best
to only use this for inportant messages, such as the muse going
down.

### @boot &lt;player&gt;
disconnect &lt;player&gt; from the game. if there are more than one
&lt;player&gt;s connected, disconnect the one that was connected
most recently.

### @chownall &lt;player&gt;=&lt;newplayer&gt;
change the ownershi pof all &lt;player&gt;'s objects to &lt;newplayer&gt;.

### @ctrace
trace all concentrator connections. there is no concentrator
client right now, so this is sorta useless.

### @class &lt;player&gt;=&lt;new class&gt;
change &lt;player&gt;'s class to &lt;new class&gt;. see doc/CLASSES.

### @dbck
do various things to make sure the database is all in
order.

### @dbtop [&lt;catagories&gt;]
show highest users of space; catagories are shown with
just plain '@dbtop'.

### @dump
checkpoint the database onto disk. (if you try to do this
at the same time that muse is doing an automatic dump,
it may give an error (db/mdb.#xxx#: no such file or
directory). it will have saved the database anyways,
though.)

### @empower &lt;player&gt;=&lt;power&gt;:&lt;class&gt;
give &lt;player&gt; the power of &lt;power&gt; to act on &lt;class&gt;.
&lt;power&gt; can be one of the powers defined in powers.h
&lt;class&gt; can be one of:
yes: always
yeseq: for equal and lower classes
yeslt: for lower classes
no: never; take away the power.

### @mailhuh
Mail out huh logs; this isn't currently tested very much.

### @newpassword &lt;player&gt;=&lt;password&gt;
change &lt;player&gt;'s password to &lt;password&gt;. if you're thinking
of using this to change your own password, don't. use
@password instead. passwords are plaintext after @newpassword.

### @nuke &lt;player&gt;
destroy &lt;player&gt;. doesn't work if they own stuff; need to
@wipeout first.

### @pbreak
show various statistics on how many people are in which class.

### @pcreate &lt;name&gt;=&lt;password&gt;
only effective if you have WCREAT defined. creates a new
character, name &lt;name&gt;, password &lt;password&gt;.

### @poor &lt;value&gt;
make everyone's credits change to &lt;value&gt;. this isn't very
good to do.

### @purge
same as @dbck, but doesn't do quite as much stuff.

### @reboot
Reboot the muse. easier than @shutdown and then going
and restarting the wd.

### @shutdown
shut down the muse. probably would be a good thing to
@broascast telling people about this first.

### @wipeout &lt;player&gt; type=&lt;type&gt;
wipe out all possessions of &lt;player&gt; of type &lt;type&gt;.
&lt;type&gt; can be:
all: wipeout all &lt;player&gt;'s possessions
rooms: wipeout all rooms
exits: wipeout all exits
objects: wileout all objects
