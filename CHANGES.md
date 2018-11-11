# CHANGES
This is the remnants of the changelog instituted beginning with version *1.8a4*

## Changes since revision 1.9f3:   Belisarius Smith <bsmith@belisariussmith.com>

- Took care of all compiler warnings (with -Wall turned on)

- Created a new header file "tinymuse.h" to hold universal or generic
  data & definitions for the project. 

- Fixed 'right-hand operand of comma expression has no effect' issues with 
  s_Pennies macro in db.h (likely a castoff from leftover legacy code)
  which was affecting several source files.

- Explicitly declared nrecur as 'static int' in free_get() from destroy.c

- Removed pointless double assignments in dbtop.c

- Added <crypt.h> to various files using crypt() and cleared any implicit
  declaration warnings.

- Casts made for improper conversions to and from chars, ints, and pointers

- Cleared vast amount of warnings due to misleading indentations, ambiguous 
  if/for/else statements, implicit if/for/else statements, and also from
  multiple nested (ugh!) dangling elses.

- Cleared unused variables

- Commented out code that did nothing (likely deprecated or legacy code that
  was either used at some point or was meant to be used in the future).

- Updated codebase to modern variable arguments and to use <stdarg.h> 
  instead of the deprecated <varargs.h>

- Added numerous comments to explain code behavior

- Renamed many variables to actually be useful.
- f    ->   file
- i    ->   db_obj_id
- etc.

- Renamed various functions to more appropriate names:
- shovechars()      ->   mud_loop()
- load_more_db()    ->   load_database()
- etc. 

- Converted old-style (K&R) C-type declaration syntac for parameters to
  the new modern style

- Removed various goto statements and labels

- Upgraded the major version number to 2 (2.0) to signify the breadth of 
  the overhaul applied to the project.

- Cast &addr_len to (socklen_t \*) inside accept() from new_connection()
  in bsd.c and &namelen to (socklen_t \*) inside getpeername() from 
  open_sockets() in bsd.c among other files

- Began cleaning up useless and deprecated old STDC style function
  declarations and the accompanying macros.

- Removed deprecated reference to zresult in do_move() from move.c

- Numerous implicit int type variable declarations made explicit

- Renamed BUFFER_LEN to MAX_CMD_BUF, then moved it, and associated define
  to tinymuse.h from interface.h ; updated all references to new one

- Fixed some typos

- Removed all calls to deprecated tprintf().

- Moved tinymuse code into a single directory, and renamed some files

- Removed all references to unnecessary &amp; dangerous code: exec_allow and do_exec()

- Removed pointless database compression tool and code

- Removed watchdog (**wd**) tool, best to leave such things to admins with scripts

- Renamed *netmuse* to **tinymuse** 

- Changed default to allowing people to create new characters

- Changed execution location from confusing run/ sub-directory to top-level directory. Server
  is now booted up by executing _# bin/tinymuse_

- Added check on database object loading so that server will no longer crash if invalid
  atrdef is used in the object

- Disabled quota check

## Changes since revision 1.9f2:		Mark Eisenstat <meisen@musenet.org>

- Changed wd to remove the 'muse_pid' file when trying to restart the MUSE.
  While this is a good thing when restarting after a crash, wd will also never
  check that the MUSE is not already running anymore - so -never- run two
  copies of wd at the same time...

- Fixed a bug in fdiv() that allowed division by 0.

- Added functions gteq(), lteq(), gt() and lt() to the hardcode.  These are
  documented in the help file.

- Finally fixed the check_lockout() code.  On most systems, gethostbyname()
  uses static buffers.  As MUSE was using this function more than once within
  its code, the buffer was being overwritten inappropriately on most systems
  (excluding Linux, which is why I never knew about it before).

- MUSE now works under SunOS.  Don't ask me why... I think it was the lockout
  code that was spooking it.

- The 'to' command has been added.  *note*:  config/config.h has changed for
  this to add a 'TO_TOKEN' to the list of one-character tokens.

- @cpage has been fixed to not crash the MUSE when an invalid concid is
  supplied.

- Captions will now be shown in inventory lists.

- Objects and non-aliased players, when paging, will have their dbref # show
  in the alias field.

- A couple miscellaneous warning messages fixed...

## Changes since revision 1.9f1:		Mark Eisenstat <meisen@musenet.org>

- The comsystem was a big mistake, to put it lightly... it's been removed and
  a new one, written by Redone (see credits.h) has been inserted to replace it.
  Although it doesn't have as many features, these will hopefully be inserted
  later.  The database version has been upgraded to 15 to remove old comsystem
  directives, and the help file has been updated.

- The +clist command has been removed.  (see help +channel)

- The +ctitle command has been added.  (see help +ctitle)

- Changed the level of function recursion from 10000 to 1000.  This helps to
  avoid some recursive function bugs.

- Changed several functions to have limits on the values used.  Here they are:

  Function			Limits:		arg1		arg2
  ----------------------------------------------------------------------------
  arccos					-1 to 1
  arcsin					-1 to 1
  arctan					-1 to 1
  cos						-1 to 1
  exp						-99 to 99
  pow						-999 to 999	-99 to 99
  sin						-1 to 1
  tan						-1 to 1
  
  These values may not be mathematically correct or acceptable on all systems.
  They work on my computer, so I assume they will probably work on some others,
  but that's the best I can do.  If anyone would like to suggest values for
  maximums/minimums for these or other functions, just drop me some email...

- Added a quick 'make clean' area to domakefile.  It's about time, eh?
  Also added 'make distclean' to remove everything clean does plus the
  makefiles.  NOTE:  Neither of the two will remove 'funcs.c', which is
  produced from 'funcs.c.gp', because a lot of people don't have/know where
  to find gperf.  (for those that want to know:  prep.ai.mit.edu:/pub/gnu,
  file name cperf\*)

## Changes since revision 1.9f0:		Mark Eisenstat <meisen@musenet.org>

- Fixed nalloc.c to properly free memory initially glurped after a reboot.
  This was a bug that was -just- fixed by Tabok so it wasn't available 
  before this.

- Made the passwords in the initial database text words, not encrypted,
  originally, as some systems seem to use different 'versions' of crypt() that
  won't decode the Linux-created encrypted passwords.

- Removed the 'files' directory from the source.  Why was it there?  Who
  knows.  The editor was buggy and broken so it's gone... (that's what was in
  the files directory but it wasn't being built into the MUSE anyway).

## Changes since revision 1.9b3:		Mark Eisenstat <meisen@musenet.org>

- A line I forgot to take out at the end of 'conf.h' was removed.  Oops.

- Added a +clist command to view the current channels/aliases being used.
  Channels without aliases will show aliases as (null).  (actually they -are-
  null, the MUSE displays nulls as (null)).

- Fixed a bug in @swap when swapping player dbrefs - the player doing the
  swapping is now notified of the end result of the swap, regardless of his/
  her new dbref.

## Changes since revision 1.9b2:		Mark Eisenstat <meisen@musenet.org>

- The 'huh?' message has been changed to something a bit less annoying.
  The player is only told how to get help if their 'newbie' flag is set.

- A bug in the colour routines that sometimes removed the last character of
  a message if it was a '"' has been fixed.

- A bug in the classname lookup routines was fixed.  Now, both long and short
  class names can be used with commands such as @class (and giving an invalid
  classname will go longer crash the MUSE).

- The match routines have been fixed so that matches aren't affected by
  any colour tokens used in the string.  (|RED|foo = foo = |BLUE,BLINK|foo)

- Spoofing also isn't affected by colours anymore. (|RED|player = player)

- NEW COMSYSTEM!  This is a nice new feature allowing titles, aliases, and
  switching on and off of channels.  The new commands are documented in the
  help for +channel.  The database version has been upgraded to 14 for this
  fix.  Also, peoples' existing channels will not be carried over to the new
  system - the 'channel' attribute has become only meaningful for the '='
  command (it is now settable by users) as a default channel.

## Changes since revision 1.9b1:		Mark Eisenstat <meisen@musenet.org>

- A couple tiny fixes to the @unhide code.  Nothing major.  Just shows a
  message if you try to @unhide while not hidden.

- You can no longer +mail players that are @lpaged against you.  Before,
  you could, but they wouldn't see the message telling them they had mail.
  That was really silly as they could me mail-spammed without even knowing
  it.

- There is now the option in the 'config.c' file of having old, read +mail
  deleted after a certain number of seconds since it was read.

- @cpage <concid>=<msg> added.  Allows someone with POW_BROADCAST to send a
  message to any concid online (including those sitting at the welcome
  screen).

- That fix to make new objects/exits/rooms !visible on creation never 
  actually got coded.  Ooooops.  It is now...

- Gah!  Before this release, there was a -major- bug in do_nologins() that
  basically allowed anyone to use the command!  If you're running anything
  below 1.9b2 and don't want to switch to b2, at -least- fix this bug as
  it's very dangerous.

## Changes since revision 1.9b0:		Mark Eisenstat <meisen@musenet.org>

- A lot of you are probably wondering why I put my name up there... it isn't
  for vanity, it's because I don't want you going to someone else and getting
  referred.  :)  When someone else takes over the project, their name will
  hopefully be up there, assuming they continue to write this file as I am.

- Re-organized 'powerlist.c' due to a heap of requests to do so.  The rows
  are explained and columns actually make perfect sense now.  Unfortunately,
  if you modified any of the powers, you'll have to re-do the modifications.
  *NOTE* The powers in 'powers.h' have been left in their original order.
  Do -not- renumber them or your players in your existing databases will 
  end up with powers they shouldn't have...

- Changed the database loading code to run -much- faster than before.  

- I forgot to mention in the 1.9b0 CHANGES:  Put in a check for the BSD
  equivalent of SIGCLD, 'SIGCHLD'.  There is no SIGCLD in BSD UNIX and it
  choked when trying to compile 'bsd.c'.

- Moved all class-related functions into 'muse/class.c'.  This includes the
  actual class names, long and short, plus the routines used to convert
  names to numbers and back.

- The true number of unlinked exits is finally displayed ont the dbinfo
  channel.

- New @dbtop selection 'connects':  ranks the top 30 users of the MUSE by
  number of connections.

- The WHO list from the login screen no longer tries to show the colour
  codes.  Oops.

- Fixed 'config/config.h' - it had a '#define ALPHA' in it that definitely
  shouldn't have been there.

- A really, really silly thing... if a user is at its home, and tries to
  go home, it'll tell them that they're already there.  Duh.  :)

- @hide <person>, @unhide <person> - hide or unhide from <person>.  Both of
  these commands without arguments will still hide or unhide you from 
  everyone.

## Changes since revision 1.9a4:		Mark Eisenstat <meisen@musenet.org>

- BETA release!

- MIDRES has been removed due to large-scale incompatibility.  I've put in
  libident v0.17a instead.  The method of compiling the MUSE hasn't changed
  at all - just do the same things.  Don't run midres as it won't be there.

- Cleaned up the code to remove all warnings I possibly could remove.

- Ability of users to set their own '@users' attribute has been removed.
  This is a very dangerous bug allowing anyone on someone's @user list to
  @su to them.

## Changes since revision 1.9a3:		Mark Eisenstat <meisen@musenet.org>

- Documentation on the new colours / WHO list colour.  Read 'help ANSI' for
  more.

- The communications system now allows hidden users on channels privacy and
  instead lists the number of hidden users on the channel when doing a 'who'
  on that channel.

## Changes since revision 1.9a2:		Mark Eisenstat <meisen@musenet.org>

- I left something in from OceanaMUSE's code that makes Guests' @adisconnect
  'CLEARCOM'.  Oops!  This has been removed...

- New colour system!  There is currently no 'helptext' documentation for it,
  but it goes like this:  

      |COLOUR[,BRIGHT][,BLINK][,REVERSE]|

  where colour is BLUE, RED, GREEN etc, and the BRIGHT, BLINK and REVERSE are
  optional keywords.  Try them out...
  
- @ljoin has been removed due to complaints about it.

- POW_STEAL now allows negative credits to be given using 'give'.

## Changes since revision 1.9a1:		Mark Eisenstat <meisen@musenet.org>

- Increased the variable 'MAX_HITS' in src/muse/nalloc.c to 250. This is the
  number of commands that the MUSE will wait before trying to cut the 'glurp'
  memory size in half.  It was set to 5 before, which is not good for all 
  MUSEs - in fact, very bad for some.  This way, the MUSE will wait 250
  commands, which will be a lot for small MUSEs, but small MUSEs won't glurp
  much anyway.

- Fixed the initial database - it was half version 11, half version 12. 
  Whoops!  Sorry about that.  For anyone experiencing a problem with the
  initial database in v1.9a0 or v1.9a1, change the very first line to @12 
  instead of @11.

## Changes since revision 1.9a0:		Mark Eisenstat <meisen@musenet.org>

- Fixed @newpassword to encrypt the password when assigning it to the player.
  Before, the player's 'password' attribute was not encrypted after a 
  @newpassword until the database was saved and re-loaded.

- @aparent, @aunparent commands - these commands are now executed by the 
  *player*, not by the parent.

- the '@lfollow' lock will not allow anyone to follow you if it is blank.

- Messages on \*log_io have been changed to show 'concid' information instead
  of 'descriptor' information as concid is more useful (and maps more easily
  to specific connections).

- Yes, there -was- an INVISIBLE flag.  It was a mistake.  It's now gone. 
  Such flags are not always adviseable and should never be used for indecent
  practices such as spying on users.

## Changes since revision 1.8a4:           Mark Eisenstat <meisen@musenet.org>

- Changed a lot of silly editing by shkoo back to its original form (perm.
  denied messages, miscellaneous messages/errors from commands)

- Condensed the initial database (muse1.9a0/run/db/initial.mdb).  The initial
  password for 'root' or 'God' is "mike", and the original password for
  'Wizard' is "fooblee".  Also, the Universal Zone is now #5 and the Universal
  Functions are #4.

- DIGFROM_OK flag.  Allows any user to @open an exit from the room or thing 
  the flag is set on.

- Parent, AParent, OParent, UnParent, AUnParent, OUnParent attributes - let 
  you define messages/actions to be shown/taken when someone @addparents or
  @delparents from a parent.

- +mail Clear (case-sensitive) - same as '+mail delete' then '+mail purge'.

- @slock on a zone will affect every room in that zone that is set AUDITORIUM.

- The 'follow <person>' command will let you follow someone around anywhere
  they go.  This is controllable using the @lfollow lock.

- @lockout <class> - blocks connections from players below class <class>.

- HAVEN attribute flag - attributes set HAVEN cannot be triggered.

- 'lastsite' attribute is automatically set at login.

- Only connected people can be @forced.

- POW_NOSLAY fixed to work properly.  (is this really needed?)

- +mail will not show new mail from an @lpaged person.

- @aidesc, @oidesc - Same as @adesc, @odesc but used with @idesc (description
  inside an object).

- @class \*person, @nopow_class \*person, WHO, class() work for everyone.

- When a player is hidden, the player appears to be disconnected, and pages
  to that player result in '<player> is either hidden or not connected'.

- Channels 'Connections' and '\*connect' added.

- @cemit <channel>=<message> - emote to a channel.  Needs POW_SPOOF.

- @wemit <message> - broadcast an emote MUSE-wide.  Needs POW_BROADCAST.

- Group ownership (Leader attribute) - allows a group 'leader' to set the 
  group's @users.

- 'visible' flag not set on object creation.

- @announce-pose ( @ann :foofs. = From across the MUSE, <person> foofs. )

- People with POW_ANNOUNCE don't get their flags shown when they @announce.

- Ability to disallow @announce per-person (the ANNOUNCE flag)

- Ability to disallow +com / +ch per-person (the +COM flag)

- Powers added:  BEEP - allows person to use the %a (beep) substitution.
                GHOST - allows person to @gethost guests.
               
- Powers re-added:  HOST (hostnames), BACKSTAGE (see dbrefs on everything)

- %r, %t, %a - newline, tab and beep, respectively.  (see POW_BEEP)

- WHO_FLAGS - Class re-added to the WHO list.

- The WHO list is now sorted by ontime again, rather than idle time.

- MIDRES code insertion.  This utility must be run before the MUSE is started
  up.  It provides 'ident' support for the MUSE.  The ident names appear in 
  many useful fields.

- The 'helptext' file is once again in this distribution.  Why was it removed?
  Who knows??
