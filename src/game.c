///////////////////////////////////////////////////////////////////////////////
// game.c                                                       TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: game.c,v 1.40 1994/02/18 22:40:50 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/wait.h>

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "credits.h"

#include "admin.h"

#define my_exit exit

// use this only in process
//
//   _command 
#define Matched(string) { if(!string_prefix((string), command)) goto bad; }

#define arg2 do_argtwo(player,rest,cause,buff)
#define argv do_argbee(player,rest,cause,arge,buff)

// External References
extern void message_ch_raw(dbref player, char *msg);
extern char ccom[BUF_SIZE];
extern int exit_status;
extern dbref cplr;

// Local Global Declarations 
FILE *command_log;

int ndisrooms;
static int epoch = 0;
char dumpfile[200];
int reserved;
int depth  = 0;   // excessive recursion prevention 
int unsafe = 0;

dbref speaker = NOTHING;

static int atr_match P((dbref, dbref, int, char *));
static void no_dbdump P((void));
static int list_check P((dbref, dbref, int, char *));

static void notify_except P((dbref, dbref, char *));

static void do_dump(dbref player)
{
    if (power(player, POW_DB))
    {
        send_message(player, "Database @dumped.");
        fork_and_dump();
    }
    else
    {
        send_message(player, "Sorry, you are in a no dumping zone.");
    }
}

///////////////////////////////////////////////////////////////////////////////
// report()
///////////////////////////////////////////////////////////////////////////////
//    Dump report data into error log 
///////////////////////////////////////////////////////////////////////////////
void report()
{
    char repbuf[BUF_SIZE*5];

    log_error("*** Reporting position ***");

    sprintf(repbuf, "Depth: %d Command: %s", depth, ccom);
    log_error(repbuf);

    sprintf(repbuf, "Player: %d location: %d", cplr, db[cplr].location);
    log_error(repbuf);

    log_error("**************************");
}

static void do_purge(dbref player)
{
    if (power(player, POW_DB))
    {
        fix_free_list();
        send_message(player, "Purge complete.");
    }
    else
    {
        send_message(player,perm_denied());
    }
}

// this is where des_info points 
void dest_info(dbref thing, dbref tt)
{
    char buff[BUF_SIZE];

    if (thing == NOTHING)
    {
        if (db[tt].name)
        {
            sprintf(buff, "You own a disconnected room, %s(#%d)", db[tt].name, tt);
            send_message(db[tt].owner, buff);
        }
        else
        {
            report();
            log_error("no name for room");
        }
        return;
    }

    switch(Typeof(thing))
    {
        case TYPE_ROOM:
              // Inform all players the room has gone away 
              notify_in(thing, 0, "The floor disappears under your feet, you fall through NOTHINGness and then:");
              break;
        case TYPE_PLAYER:
            // Show  where they've arrived 
            enter_room(thing, HOME);
            break;
    }
}

static void notify_nopup(dbref player, char *msg)
{
    static char buff[BUF_SIZE*2],*d;

    if (player < 0 || player >= db_top)
    {
        return;
    }

    if (depth++ > 7)
    {
        depth--;
        return;
    }

    if (db[player].owner != player)
    {
        strcpy(buff, msg);

        if (*(d = atr_get(player, A_LISTEN)) && wild_match(d, buff))
        {

            if (speaker != player)
            {
                did_it(speaker, player, 0, NULL, 0, NULL, A_AHEAR);
            }
            else
            {
                did_it(speaker, player, 0, NULL, 0, NULL, A_AMHEAR);
            }

            did_it(speaker, player, 0, NULL, 0, NULL, A_AAHEAR);

            // also pass the message on 
            // Note: not telling player protects against two forms of recursion 
            // player doesn't tell itself (as container) or as contents 
            // using teleport it is possible to create a recursive loop 
            // but this will be terminated when the depth variable exceeds 30 
            if (db[speaker].location != player)
            {
                notify_in(player, player, buff);
            }
        }
        // now check for multi listeners 
        if (speaker != player)
        {
            atr_match(player, speaker, '!', msg);
        }
    }

    depth--;
}

void send_message(dbref player, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[MAX_BUF*2];

    vsprintf(buffer, format, args);

    va_end(args);

    if ((player < 0) || (player >= db_top))
    {
        return;
    }

    message_ch_raw(player, buffer);

    if (db[player].flags & PUPPET && db[player].owner != player)
    {
        sprintf(buffer, "%s> %s", db[player].name, buffer);
        message_ch_raw(db[player].owner, buffer);
    }
}

// Notify the users of 'group' of something... 
void notify_group(dbref group, char *message)
{
    char buf[BUF_SIZE];
    char *s = buf;
    char *z;

    strcpy(buf, atr_get(group, A_USERS));

    while ((z = parse_up(&s, ' ')))
    {
        int i;
        if (*z != '#') continue;
        i = atoi(z+1);
        send_message(i, message);
    }
}

// special notify, if puppet && owner is in same room don't relay 
static void snotify(dbref player, char *msg)
{
    if ((db[player].owner != player) && (db[player].flags & PUPPET)
      && (db[player].location == db[db[player].owner].location))
    {
        notify_nopup(player, msg);
    }
    else
    {
        send_message(player, msg);
    }
}

static void notify_except(dbref first, dbref exception, char *msg)
{
    if (first == NOTHING)
    {
        return;
    }

    DOLIST (first, first)
    {
        if (// ((db[first].flags & TYPE_MASK) == TYPE_PLAYER ||
            // ((db[first].flags & PUPPET) && (Typeof(first)==TYPE_THING)))
            //&& 
            first != exception)
        {
            snotify(first, msg);
        }
    }
}

void notify_in(dbref room, dbref exception, char *msg)
{
    dbref z;

    DOZONE(z, room)
    {
        send_message(z, msg);
    }

    if (room == NOTHING)
    {
        return;
    }

    if (room != exception)
    {
        snotify(room, msg);
    }

    notify_except(db[room].contents, exception, msg);
    notify_except(db[room].exits, exception, msg);
}

static void do_shutdown(dbref player, char *arg1)
{
    char buf[BUF_SIZE];

    if (strcmp(arg1, muse_name))
    {
        if (!*arg1)
        {
            send_message(player, "You must specify the name of the muse you wish to shutdown.");
        }
        else
        {
            send_message(player, "This is %s, not %s.", muse_name, arg1);
        }

        return;
    }

    sprintf(buf, "Shutdown attempt by %s", unparse_object(player, player));
    log_sensitive(buf);

    if (power(player, POW_SHUTDOWN))
    {
        sprintf(buf, "SHUTDOWN: by %s", unparse_object(player, player));
        log_important(buf);

        shutdown_flag = TRUE;
        exit_status   = STATUS_NONE;
    }
    else
    {
        send_message(player, "@shutdown is a restricted command.");
    }
}

static void do_reboot(dbref player, char *arg1)
{
    char buf[BUF_SIZE];

    if (strcmp(arg1, muse_name))
    {
        if (!*arg1)
        {
            send_message(player, "You must specify the name of the muse you wish to reboot.");
        }
        else
        {
            send_message(player, "This is %s, not %s.", muse_name, arg1);
        }

        return;
    }

    sprintf(buf, "Reboot attempt by %s", unparse_object(player, player));
    log_sensitive(buf);

    if (power(player, POW_SHUTDOWN))
    {
        sprintf(buf, "REBOOT: by %s", unparse_object(player, player));
        log_important(buf);

        shutdown_flag = TRUE;
        exit_status   = STATUS_REBOOT;
    }
    else
    {
        send_message(player, "@reboot is a restricted command.");
    }
}

static void dump_database_internal()
{
    char tmpfile[BUF_SIZE*2];
    FILE *f;

    sprintf(tmpfile, "%s.#%d#", dumpfile, epoch - 3);
    unlink(tmpfile);            // nuke our predecessor 
    sprintf(tmpfile, "%s.#%d#", dumpfile, epoch);

    if ( (f = fopen(tmpfile, "w")) != NULL )
    {
        db_write(f);
        fclose(f);
        unlink(dumpfile);

        if ( link(tmpfile, dumpfile) < 0 )
        {
            perror(tmpfile);
            no_dbdump();
        }

        sync();
    }
    else
    {
        no_dbdump();
        perror(tmpfile);
    }
}

void panic(char *message)
{
    char buf[BUF_SIZE*2];
    FILE *file;
    int i;

    sprintf(buf, "PANIC!! %s", message); // kludge! 
    log_error(buf);

    report();

    // turn off signals 
    for (i = 0; i < NSIG; i++)
    {
        signal(i, SIG_IGN);
    }

    // shut down interface 
    emergency_shutdown();

    // dump panic file 
    sprintf(buf, "%s.PANIC", dumpfile);

    if ((file = fopen(buf, "w")) == NULL)
    {
        perror("CANNOT OPEN PANIC FILE, YOU LOSE");
        exit_nicely(136);
    }
    else
    {
        sprintf(buf, "DUMPING: %s", buf);
        log_io(buf);

        db_write(file);
        fclose(file);

        sprintf(buf, "%s (done)", buf);
        log_io(buf);

        exit_nicely(136);
    }
}

void dump_database()
{
    char buf[BUF_SIZE];
    epoch++;

    sprintf(buf, "DUMPING: %s.#%d#", dumpfile, epoch);
    log_io(buf);

    dump_database_internal();
    sprintf(buf, "DUMPING: %s.#%d# (done)", dumpfile, epoch);
    log_io(buf);
}

///////////////////////////////////////////////////////////////////////////////
// free_database()
///////////////////////////////////////////////////////////////////////////////
//    Free all the objects, and everything. for malloc debugging. 
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
void free_database()
{
    int i;

    for (i = 0; i < db_top; i++)
    {
        free (db[i].name);

        if (db[i].parents)
            free(db[i].parents);

        if (db[i].children)
            free(db[i].children);

        if (db[i].pows)
            free(db[i].pows);

        if (db[i].atrdefs)
        {
            struct atrdef *j, *next = NULL;

            for (j=db[i].atrdefs; j; j = next)
            {
                next = j->next;
                free(j->a.name);
                free(j);
            }
        }

        if (db[i].list)
        {
            ALIST *j, *next=NULL;

            for (j = db[i].list; j; j = next)
            {
                next = AL_NEXT(j);
                free(j);
            }
        }
    }

    free(db-5);
}

void fork_and_dump()
{
    char buf2[BUF_SIZE];
    int child;
    static char buf[100] = "";

    // first time through only, setup dump message 
    if ( *buf == '\0' )

    epoch++;

    sprintf(buf2, "CHECKPOINTING: %s.#%d#", dumpfile, epoch);
    log_io(buf2);

    child = fork();

    if ( child == -1 )
    {
        child = vfork(); // not enough memory! or something.. 
    }

    wait(0);

    if (child == 0)
    {
        // in the child 
        close(reserved);        // get that file descriptor back 
        dump_database_internal();
        _exit(0);
    }
    else if (child < 0)
    {
        perror("fork_and_dump: fork()");
        no_dbdump();
    }
}

static void no_dbdump()
{
    do_broadcast(root, "Database save failed. Please take appropriate precautions.", "");
}

int init_game(char *infile, char *outfile)
{
    char buf[BUF_SIZE];
    FILE *f;
    int a;
    depth = 0;

    for (a = 0; a < 10; a++)
    {
        wptr[a] = NULL;
    }
    // reserved=open("/dev/null",O_RDWR); // #JMS#  

    if ((f = fopen(infile, "r")) == NULL)
    {
        return -1;
    }

    // ok, read it in 
    sprintf(buf, "LOADING: %s", infile);
    log_important(buf);

    fflush(stdout);
    db_set_read(f);
    sprintf(buf, "LOADING: %s (done)", infile);
    log_important(buf);
    // everything ok 

    // initialize random number generator 
    srandom(getpid());
    // set up dumper 
    strcpy(dumpfile,outfile);
    init_timer();

    return 0;
}

// the two versions of argument parsing 
static char *do_argtwo(dbref player, char *rest, dbref cause, char *buff)
{
    exec(&rest, buff, player, cause, 0);

    return(buff);
}

static char **do_argbee(dbref player, char *rest, dbref cause, char *arge[], char *buff)
{
    int a;

    for(a = 1; a < MAX_ARG; a++)
    {
        arge[a] = (char *) parse_up(&rest,',');
    }
    // rest of delimiters are ,'s 

    for (a = 1; a < MAX_ARG; a++)
    {
        if (arge[a])
        {
            exec(&arge[a],buff,player,cause, 0);
            strcpy(arge[a]=(char *)newglurp(strlen(buff)+1),buff);
        }
    }

    return(arge);
}

void process_command(dbref player, char *command, dbref cause)
{
    char *arg1;
    char *q;			// utility 
    char *p;			// utility
    // char args[MAX_ARG][1
    char buff[BUF_SIZE];
    char buff2[BUF_SIZE];
    char *arge[MAX_ARG];        // pointers to arguments (null for empty) 
    char unp[BUF_SIZE];         // unparsed command 
    char pure[BUF_SIZE];        // totally unparsed command 
    char pure2[BUF_SIZE];
    char *rest, *temp;
    int match, z=NOTHING;
    dbref zon;
    // general form command arg0=arg1, arg2...arg10 
    int slave = IS(player, TYPE_PLAYER, PLAYER_SLAVE);
    int is_direct = 0;

    if (player == root)
    {
        if (cause == NOTHING)
        {
            sprintf(buff, "(direct) %s",command);
            log_is_root(buff);
        }
        else
        {
            sprintf(buff, "(cause %d) %s",cause, command);
            log_is_root(buff);
        }
    }

    if (cause == NOTHING)
    {
        is_direct = 1;
        cause = player;
    };

    if (is_direct && Typeof(player)!=TYPE_PLAYER && *atr_get(player,A_INCOMING))
    {
        wptr[0] = command;
        did_it(player, player, NULL, NULL, NULL, NULL, A_INCOMING);
        atr_match(player, player, '^', command);
        wptr[0] = 0;

        return;
    }

    inc_pcmdc(); // increment command stats 

    temp = command;

    // skip leading space 
    while (*temp && (*temp==' '))
    {
        temp++;
    }

    // skip firsts word 
    while (*temp && (*temp!=' '))
    {
        temp++;
    }

    // skip leading space 
    if (*temp)
    {
        temp++;
    }

    strcpy(pure,temp);

    while(*temp && (*temp!='=')) temp++;

    if (*temp)
    {
        temp++;
    }

    strcpy(pure2, temp);
    func_zerolev();
    clearglurp();
    depth = 0;

    if (command == 0)
    {
        abort();
    }

    // Access the player 
    if (player == root && cause != root)
    {
        return;
    }

    // robustify player 
    if ((player < 0) || (player >= db_top))
    {
        sprintf(buff, "process_command: bad player %d", player);
        log_error(buff);

        return;
    }

    speaker = player;

    {
        static short counter;

        if (command_log == NULL)
        {
            command_log = fopen("run/logs/commands", "w");
            setbuf(command_log, NULL);
        }

        if (command_log == NULL)
        {
            command_log = stderr;
        }

        if (db[player].location < 0 || db[player].location >= db_top)
        {
            fprintf(command_log, "COMMAND from %s(%d) in #%d(ACK!): %s\n", db[player].name, player, db[player].location, command);

            sprintf(buff, "illegal location: player %d location %d",player,db[player].location);
            log_error(buff);

            moveto(player, player_start);

            return;
        }

        fprintf(command_log, "COMMAND from %s(%d) in %s(%d): %s\n", db[player].name, player, db[db[player].location].name, db[player].location, command);

        fflush(command_log);

        if (counter++ > 100)
        {
            counter = 0;

            if (command_log != stderr)
            {
                fclose(command_log);
            }

            command_log = NULL;
            unlink("run/logs/commands~");
            rename("run/logs/commands", "run/logs/commands~");
        }
    }

    if ((db[player].flags & PUPPET) && (db[player].flags & DARK))
    {
        char buf[BUF_SIZE*2];

        sprintf(buf, "%s>> %s", db[player].name, command);
        message_ch_raw(db[player].owner, buf);  // Wouldn't regular message_ch() work? -- Belisarius
    }

    // eat leading whitespace 
    while(*command && isspace(*command)) command++;

    // eat extra white space 
    q = p = command;

    while(*p)
    {
        // scan over word 
        while (*p && !isspace(*p))
            *q++ = *p++;

        // smash spaces 
        while (*p && isspace(*++p))
        ;

        if (*p)
        {
            // add a space to separate next word 
            *q++ = ' ';
        }
    }

    // terminate 
    *q = '\0';

    // important home checking comes first! 
    if (strcmp(command, "home") == 0)
    {
        do_move(player,command);

        if (command_log)
        {
              fputc('.', command_log);
              fflush(command_log);
        }

        return;
    }

    if (!slave && try_force(player, command)) /*||
					     ((Typeof(player)!=TYPE_PLAYER) && (Typeof(player)!=TYPE_THING)))*/ {
                                               if(command_log) {
                                                 fputc('.',command_log);
                                                 fflush(command_log);
                                               }
                                               return;
                                             }

    // check for single-character commands 
    if (*command == SAY_TOKEN)
    {
        do_say(player, command+1, NULL);
    }
    else if(*command == POSE_TOKEN)
    {
        do_pose(player, command+1, NULL, 0);
    }
    else if(*command == NOSP_POSE)
    {
        do_pose(player, command+1, NULL, 1);
    }
    else if(*command == COM_TOKEN)
    {
        do_com(player, "", command+1);
    }
    else if(*command == TO_TOKEN)
    {
        do_to(player, command+1, NULL);
    }
    else if(can_move(player, command))
    {
        // command is an exact match for an exit 
        do_move(player, command);
    }
    else
    {
        strcpy(unp,command);
        // parse arguments 

        // find arg1 
        // move over command word 
        for (arg1 = command; *arg1 && !isspace(*arg1); arg1++)
        ;

        // truncate command 
        if (*arg1)
        {
            *arg1++ = '\0';
        }

        // move over spaces
        while (*arg1 && isspace(*arg1))
        {
            arg1++;
        }

        strcpy(buff, arg1);                     // save the combined arguments for later 

        arge[0] = (char *)parse_up(&arg1, '='); // first delimiter is ='s 
        rest = arg1;                            // either arg2 or argv 

        if (arge[0])
        {
              exec(&arge[0],buff2,player,cause, 0);
        }

        arg1 = (arge[0]) ? buff2 : "";

        if (slave)
        {
            switch(command[0])
            {
                case 'l':
	            Matched("look");
                    do_look_at(player, arg1);
                    break;
            }
        }
        else
        {
            switch(command[0])
            {
                case '+':
                    switch(command[1])
                    {
                        case 'a':
                        case 'A':
                            Matched("+away");
                            do_away(player, arg1);
                            break;
                        case 'c':
                        case 'C':
                            switch (command[2])
                            {
                                case 'o':
                                case 'O':
                                case '\0':
	                            Matched("+com");

                                    if (Guest(player))
                                    {
                                        send_message(player, guest_fail());
                                        return;
                                    };

                                    do_com(player, arg1, arg2);
                                    break;
                                case 'm':
                                case 'M':
                                    Matched("+cmdav");
                                    do_cmdav(player);
                                    break;
                                case 'h':
                                case 'H':
                                    Matched("+channel");

                                    if (Guest(player))
                                    {
                                        send_message(player, guest_fail());
                                        return;
                                    };

                                    do_channel(player, arg1);
                                    break;
                                case 't':
                                case 'T':
                                    Matched("+ctitle");
                                    do_ctitle(player, arg1);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'h':
                        case 'H':
                            Matched("+haven");
                            do_haven(player, arg1);
                            break;
                        case 'i':
                        case 'I':
                            Matched("+idle");
                            do_idle(player, arg1);
                            break;
                        case 'l':
                        case 'L':
                            switch (command[2]) {
                                case 'a':
                                case 'A':
                                    Matched("+laston");
                                    do_laston(player, arg1);
                                    break;
                                default: goto bad;
                            }
                            break;
                        case 'm':
                        case 'M':
                            switch(command[2]) {
                                case 'a':
                                case 'A':
                                    Matched("+mail");
                                    do_mail(player, arg1, arg2);
                                    break;
                                default: goto bad;
                            }
                            break;
                        case 'u':
                        case 'U':
                            Matched("+uptime");
                            do_uptime(player);
                            break;
                        case 'v':
                        case 'V':
                            Matched("+version");
                            do_version(player);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case '@':
                    switch(command[1])
                    {
                        case 'a':
                        case 'A':
                            switch(command[2]) {
                                case 'd':
                                case 'D':
                                    Matched("@addparent");

                                    if (Guest(player))
                                    {
                                        send_message(player, guest_fail());
                                        return;
                                    };

                                    do_addparent (player, arg1, arg2);
                                    break;
                                case 'l':
                                case 'L':
                                    Matched("@allquota");
                                    if (!is_direct) goto bad;
                                    do_allquota(player, arg1);
                                    break;
                                case 'n':
                                case 'N':
                                    Matched("@announce");
                                    do_announce(player, arg1, arg2);
                                    break;
                                case 't':
                                case 'T':
                                    Matched("@at");
                                     if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_at(player, arg1, arg2);
                                    break;
                                case 's':
                                case 'S':
                                    Matched("@as");
                                     if (Guest(player)) { send_message(player, guest_fail()); return; };
                            do_as(player, arg1, arg2);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'b':
                        case 'B':
                            switch(command[2])
                            {
                                case 'r':
                                case 'R':
                                    Matched("@broadcast");
                                    do_broadcast(player, arg1, arg2);
                                    break;
                                case 'o':
                                case 'O':
                                    Matched("@boot");
                                    do_boot(player, arg1);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'c':
                        case 'C':
                            // chown, create 
                            switch(command[2])
                            {
                                case 'b':
                                case 'B':
                                    Matched("@cboot");
                                    do_cboot(player, arg1);
                                    break;
                                case 'e':
                                case 'E':
                                    Matched("@cemit");
                                    do_cemit(player, arg1, arg2);
                                    break;
                                case 'h':
                                case 'H':
                                    if ( string_compare(command, "@chownall") == 0 )
                                    {
                                        do_chownall(player, arg1, arg2);
                                    }
                                    else
                                    {
                                        switch (command[3])
                                        {
                                            case 'o':
                                            case 'O':
                                            	Matched("@chown");

                                                if (Guest(player))
                                                {
                                                    send_message(player, guest_fail());
                                                    return;
                                                };

                                              	do_chown(player, arg1, arg2);
                                          	break;
                                            case 'e':
                                            case 'E':
                                                Matched("@check");
                                                do_check (player, arg1);
                                                break;
                                            default:
                                                goto bad;
                                        }
                                    }
                                    break;
                                case 'p':
                                case 'P':
                                    Matched("@cpage");
                                    do_cpage(player, arg1, arg2);
                                    break;
                                case 'r':
                                case 'R':
                                    Matched("@create");
                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_create(player, arg1, atol(arg2));
                                    break;
                                case 't':
                                case 'T':
                                    Matched("@ctrace");
                                    do_ctrace(player);
                                    break;
                                case 'l':
                                case 'L':
                                    switch(command[3])
                                    {
                                        case 'a':
                                        case 'A':
                                            Matched("@class");
                                            if (!is_direct)
                                            {
                                                goto bad;
                                            }
                                            do_class(player, arg1, arg2);
                                            break;
                                        case 'e':
                                        case 'E':
                                            Matched("@clearmail");
                                            if (!(Wizard(player))) { send_message(player, perm_denied()); return; }
                                            clear_old_mail();
                                            break;
                                        case 'o':
                                        case 'O':
                                            Matched("@clone");
                                            if (Guest(player)) { send_message(player, guest_fail()); return; };
                                            do_clone(player, arg1, arg2);
                                            break;
                                        default:
                                            goto bad;
                                    }
                                    break;
                                case 'o':
                                case 'O':
                                    Matched("@config");
                                    do_config(player, arg1, arg2);
                                    break;
                                case 's':
                                case 'S':
                                    Matched("@cset");
                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_set(player, arg1, arg2, 1);
                                    break;
                                case 'y':
                                case 'Y':
                                    Matched("@cycle");
                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_cycle(player, arg1, argv);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'd':
                        case 'D':
                            // describe, dig, or dump 
                            switch(command[2])
                            {
                                case 'b':
                                case 'B':
                                    switch (command[3])
                                    {
                                        case 'c':
                                        case 'C':
                                            Matched("@dbck");
                                            do_dbck(player);
                                            break;
                                        case 't':
                                        case 'T':
                                            Matched("@dbtop");
                                            do_dbtop(player, arg1);
                                            break;
                                        default:
                                            goto bad;
                                    }
                                    break;
                                case 'e':
                                case 'E':
                                    switch (command[3])
                                    {
                                        case 'c':
                                        case 'C':
                                            Matched("@decompile");
                                            if (Guest(player)) { send_message(player, guest_fail()); return; };
                                            do_decompile (player, arg1, arg2);
                                            break;
                                        case 's':
                                        case 'S':
                                            switch (command[4])
                                            {
                                                case 'c':
                                                case 'C':
                                                    Matched("@describe");
                                                    do_describe(player, arg1, arg2);
                                                    break;
                                                case 't':
                                                case 'T':
                                                    Matched("@destroy");
                                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                                    do_recycle(player, arg1);
                                                    break;
                                                default:
                                                    goto bad;
                                            }
                                            break;
                                        case 'f':
                                        case 'F':
                                            Matched("@defattr");
                                            if (Guest(player)) { send_message(player, guest_fail()); return; };
                                            do_defattr(player, arg1, arg2);
                                            break;
                                        case 'l':
                                        case 'L':
                                            Matched("@delparent");
                                              if (Guest(player)) { send_message(player, guest_fail()); return; };
                                            do_delparent (player, arg1, arg2);
                                            break;
                                        default:
                                            goto bad;
                                    }
                                    break;
                                case 'i':
                                case 'I':
                                    Matched("@dig");
                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_dig(player, arg1, argv);
                                    break;
                                case 'u':
                                case 'U':
                                    Matched("@dump");
                                    do_dump(player);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'f':
                        case 'F':
                            switch(command[2])
                            {
                                case 'i':
                                case 'I':
                                    Matched("@find");
                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_find(player, arg1);
                                    break;
                                case 'o':
                                case 'O':
                                    if (string_prefix ("@foreach", command) && strlen(command)>4)
                                    {
                                        Matched("@foreach");
                                        if (Guest(player)) { send_message(player, guest_fail()); return; };
                                        do_foreach(player, arg1, arg2, cause);
                                        break;
                                    }
                                    else
                                    {
                                        Matched("@force");
                                        do_force(player, arg1, arg2);
                                        break;
                                    }
                                    default:
                                        goto bad;
                            }
                            break;
                        case 'e':
                        case 'E':
                            switch(command[2])
                            {
                                case 'c':
                                case 'C':
                                    Matched("@echo");
                                    if (Guest(player))
                                    {
                                        do_echo(player, arg1, pure2, 0);
                                    }
                                    else
                                    {
                                        do_echo(player, arg1, arg2, 0);
                                    }
                                    break;
                                case 'd':
                                case 'D':
                                    Matched("@edit");
                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_edit(player, arg1, argv);
                                    break;
                                case 'm':
                                case 'M':
                                    switch(command[3])
                                    {
                                        case 'i':
                                        case 'I':
                                            Matched("@emit");
                                            if (Guest(player))
                                            {
                                                do_emit(player, arg1, pure2, 0);
                                            }
                                            else
                                            {
                                                do_emit(player, arg1, arg2, 0);
                                            }
                                            break;
                                        case 'p':
                                        case 'P':
                                            Matched("@empower");
                                            if (!is_direct)
                                            {
                                                goto bad;
                                            }
                                            do_empower(player, arg1, arg2);
                                            break;
                                        default:
                                            goto bad;
                                    }
                                    break;
                                case 'x':
                                case 'X':
                                default:
                                    goto bad;
                            }
                            break;
                        case 'g':
                        case 'G':
                            switch(command[2])
                            {
                                case 'i':
                                case 'I':
                                    Matched("@giveto");
                                    do_giveto(player, arg1, arg2);
                                    break;
                                case 'e':
                                case 'E':
                                    Matched("@gethost");
                                    do_gethost(player, arg1);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'h':
                        case 'H':		// removes all queued commands by your objects 
                            // halt or hide 
                            switch(command[2])
                            {
                                case 'a':
                                case 'A':
                                    Matched("@halt");
                                    do_halt(player, arg2);
                                    break;
                                case 'i':		// hides player name from WHO 
                                case 'I':
                                    Matched("@hide");
                                    do_hide(player, arg1);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'i':
                        case 'I':
                            Matched("@info");
                            do_info(player, arg1);
                            break;
                        case 'l':
                        case 'L':
                            // lock or link 
                            switch(command[2])
                            {
                                case 'i':
                                case 'I':
                                    Matched("@link");
                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_link(player, arg1, arg2);
                                    break;
                                case 'o':
                                case 'O':
                                    switch (command[5])
                                    {
                                        case 'o':
                                        case 'O':
                                            Matched("@lockout");
                                            do_lockout(player, arg1);
                                            break;
                                        default:
                                            goto bad;
                                    }
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'm':
                        case 'M':
                            Matched("@misc"); // miscelanious functions 
                            do_misc(player, arg1, arg2);
                            break;
                        case 'n':
                        case 'N':
                            switch(command[2])
                            {
                                case 'a':
                                case 'A':
                                    Matched("@name");
                                    do_name(player, arg1, arg2, is_direct);
                                    break;
                                case 'c':
                                case 'C':
                                    Matched("@ncset");
                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_set(player, arg1, pure2, 1);
                                    break;
                                case 'e':
                                case 'E':
                                    switch(command[3]) {
                                        case 'c':
                                        case 'C':
                                            Matched("@necho");
                                            do_echo(player, pure, NULL, 1);
                                            break;
                                        case 'w':
                                            if(strcmp(command, "@newpassword")) goto bad;
                                            if (!is_direct) goto bad;
                                            do_newpassword(player, arg1, arg2);
                                            break;
                                        case 'm':
                                        case 'M':
                                            Matched("@nemit");
                                            do_emit(player, pure, NULL, 1);
                                            break;
                                        default:
                                        goto bad;
                                    }
                                    break;
                                case 'o':
                                case 'O':
                                    switch (command[3]) {
                                        case 'l':
                                        case 'L':
                                            Matched("@nologins");
                                            if (Guest(player)) { send_message(player, guest_fail()); return; };
                                            do_nologins(player, arg1);
                                            break;
                                        case 'o':
                                        case 'O':
                                            Matched("@noop");
                                            break;
                                        case 'p':
                                        case 'P':
                                            Matched("@nopow_class");
                                            if (!is_direct) goto bad;
                                            do_nopow_class(player, arg1, arg2);
                                            break;
                                            default: goto bad;
                                    }
                                    break;
                                case 'p':
                                case 'P':
                                    switch(command[3])
                                    {
                                        case 'e':
                                        case 'E':
                                            Matched("@npemit");
                                            do_general_emit(player, arg1, pure, 4);
                                            break;
                                        case 'a':
                                        case 'A':
                                            Matched("@npage");
                                            do_page(player, arg1, pure2);
                                            break;
                                        default:
                                            goto bad;
                                    }
                                    break;
                                case 's':
                                case 'S':
                                    Matched("@nset");
                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_set(player, arg1, pure2, is_direct);
                                    break;
                                case 'u':
                                case 'U':
                                    Matched("@nuke");
                                    if (!is_direct)
                                    {
                                        goto bad;
                                    }
                                    do_nuke(player, arg1);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'o':
                        case 'O':
                            switch(command[2])
                            {
                                case 'e':
                                case 'E':
                                    Matched("@oemit");

                                    if (Guest(player))
                                    {
                                        do_general_emit(player, arg1, pure2, 2);
                                    }
                                    else
                                    {
                                        do_general_emit(player, arg1, arg2, 2);
                                    }
                                    break;
                                case 'p':
                                case 'P':
                                    Matched("@open");
                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_open(player, arg1, arg2,NOTHING);
                                    break;
                                case 'u':
                                case 'U':
                                    Matched("@outgoing");
                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_outgoing(player, arg1, arg2);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'p':
                        case 'P':
                            switch(command[2])
                            {
                                case 'a':
                                case 'A':
                                    Matched("@password");
                                    do_password(player, arg1, arg2);
                                    break;
                                case 'b':
                                case 'B':
                                    Matched("@pbreak");
                                    do_pstats(player, arg1);
                                    break;
                                case 'C':
                                case 'c':
                                    Matched("@pcreate");
                                    do_pcreate(player, arg1, arg2);
                                    break;
                                case 'e':
                                case 'E':
                                    Matched("@pemit");
                                    if (Guest(player))
                                    {
                                        do_general_emit(player, arg1, pure2, 0);
                                    }
                                    else
                                    {
                                        do_general_emit(player, arg1, arg2, 0);
                                    }
                                    break;
                                case 'O':
                                case 'o':
                                    switch(command[3])
                                    {
                                        case 'o':
                                        case 'O':
                                            Matched("@Poor");
                                            if (!is_direct)
                                            {
                                                goto bad;
                                            }
                                            do_poor(player, arg1);
                                            break;
                                        case 'w':
                                        case 'W':
                                            Matched("@powers");
                                            do_powers(player, arg1);
                                            break;
                                        default:
                                            goto bad;
                                    }
                                    break;
                                case 'S':
                                case 's':
                                    Matched("@ps");
                                    do_queue(player);
                                    break;
                                case 'u':
                                case 'U':		/* force room destruction */
                                    Matched("@purge");
                                    do_purge(player);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'q':
                        case 'Q':
                                Matched("@quota");
                                do_quota(player, arg1, arg2);
                                break;
                        case 'r':
                        case 'R':
                                switch(command[2])
                                {
                                    case 'e':
                                    case 'E':
                                        switch(command[3])
                                        {
                                            case 'b':
                                            case 'B':
                                                Matched("@reboot");
                                                do_reboot(player, arg1);
                                                break;
                                            case 'm':
                                            case 'M':
                                                Matched("@remit");
                                                if (Guest(player))
                                                {
                                                    do_general_emit(player, arg1, pure2, 1);
                                                }
                                                else
                                                {
                                                    do_general_emit(player, arg1, arg2, 1);
                                                }
                                                break;
                                            default:
                                                goto bad;
                                        }
                                        break;
                                    case 'O':
                                    case 'o':
                                        Matched("@robot");
                                        do_robot(player, arg1, arg2);
                                        break;
                                    default:
                                        goto bad;
                                }
                                break;
                            case 's':
                            case 'S':
                                switch(command[2])
                                {
                                    case 'e':
                                    case 'E':
                                        switch(command[3])
                                        {
                                            case 'a':
                                            case 'A':
                                                Matched("@search");
                                                if (Guest(player)) { send_message(player, guest_fail()); return; };
                                                do_search(player, arg1, arg2);
                                                break;
                                            case 't':
                                            case 'T':
                                                Matched("@set");
                                                if (Guest(player)) { send_message(player, guest_fail()); return; };
                                                do_set(player, arg1, arg2, is_direct);
                                                break;
                                            default:
                                                goto bad;
                                        }
                                        break;
                                    case 'h':
                                    case 'H':
                                        switch(command[3])
                                        {
                                            case 'u':
                                                if(strcmp(command, "@shutdown")) goto bad;
                                                do_shutdown(player, arg1);
                                                break;
                                            case 'o':
                                            case 'O':
                                                Matched("@showhash");
                                                do_showhash(player, arg1);
                                                break;
                                            default:
                                                goto bad;
                                        }
                                        break;
                                    case 't':
                                    case 'T':
                                        Matched("@stats");
                                        do_stats(player, arg1);
                                        break;
                                    case 'u':
                                    case 'U':
                                        Matched("@su");
                                        if (Guest(player)) { send_message(player, guest_fail()); return; };
                                        do_su(player, arg1, arg2);
                                        break;
                                    case 'w':
                                    case 'W':
                                        switch(command[3])
                                        {
                                            case 'a':
                                            case 'A':
                                                Matched("@swap");
                                                do_swap(player, arg1, arg2);
                                                break;
                                            case 'e':
                                            case 'E':
                                                Matched("@sweep");
                                                do_sweep(player, pure);
                                                break;
                                            case 'i':
                                            case 'I':
                                                Matched("@switch");
                                                do_switch(player, arg1, argv, cause);
                                                break;
                                            default:
                                                goto bad;
                                        }
                                        break;
                                    default:
                                        goto bad;
                                }
                                break;
                            case 't':
                            case 'T':
                                switch(command[2])
                                {
                                    case 'e':
                                    case 'E':
                                        switch(command[3])
                                        {
                                            case 'l':
                                            case 'L':
                                            case '\0':
                                                Matched("@teleport");
                                                if (Guest(player)) { send_message(player, guest_fail()); return; };
                                                do_teleport(player, arg1, arg2);
                                                break;
                                            case 'x':
                                            case 'X':
                                                Matched("@text");
                                                do_text(player, arg1, arg2, NULL);
                                                break;
                                            default: goto bad;
                                        }
                                        break;
                                    case 'r':
                                    case 'R':
                                        switch(command[3])
                                        {
                                            case 'i':
                                            case 'I':
                                            case '\0':
                                                Matched("@trigger");
                                                if (Guest(player)) { send_message(player, guest_fail()); return; };
                                                do_trigger(player, arg1, argv);
                                                break;
                                            case '_':
                                                Matched("@tr_as");
                                                if (Guest(player)) { send_message(player, guest_fail()); return; };
                                                do_trigger_as(player, arg1, argv);
                                                break;
                                            default: goto bad;
                                        };
                                        break;
                                    default:
                                        goto bad;
                                }
                                break;
                            case 'u':
                            case 'U':
                                switch(command[2])
                                {
                                    case 'l':
                                    case 'L':
                                        Matched("@ulink");
                                        do_ulink(player, arg1);
                                        break;
                                    case 'n':
                                    case 'N':
                                        switch(command[3])
                                        {
                                            case 'd':
                                            case 'D':
                                                switch(command[4])
                                                {
                                                    case 'e':
                                                    case 'E':
                                                        switch(command[5])
                                                        {
                                                            case 's':
                                                            case 'S':
                                                                Matched("@undestroy");
                                                                if (Guest(player)) { send_message(player, guest_fail()); return; };
                                                                do_undestroy(player, arg1);
                                                                break;
                                                            case 'f':
                                                            case 'F':
                                                                Matched ("@undefattr");
                                                                if (Guest(player)) { send_message(player, guest_fail()); return; };
                                                                do_undefattr (player, arg1);
                                                                break;
                                                            default:
                                                                goto bad;
                                                        }
                                                        break;
                                                    default:
                                                        goto bad;
                                                }
                                                break;
                                            case 'l':
                                            case 'L':
                                                switch(command[4])
                                                {
                                                    case 'i':
                                                    case 'I':
                                                        Matched("@unlink");
                                                        if (Guest(player)) { send_message(player, guest_fail()); return; };
                                                        do_unlink(player, arg1);
                                                        break;
                                                    case 'o':
                                                    case 'O':
                                                        Matched("@unlock");
                                                        if (Guest(player)) { send_message(player, guest_fail()); return; };
                                                        do_unlock(player, arg1);
                                                        break;
                                                    default:
                                                        goto bad;
                                                }
                                                break;
                                            case 'h':
                                            case 'H':
                                                Matched("@unhide");
                                                do_unhide(player, arg1);
                                                break;
                                            case 'z':
                                            case 'Z':
                                                Matched("@unzlink");
                                                do_unzlink(player, arg1);
                                                break;
                                                default: goto bad;
                                        }
                                        break;
                                    case 'p':
                                    case 'P':
                                        Matched("@upfront");
                                        do_upfront(player, arg1);
                                        break;
                                    case 's':
                                    case 'S':
                                        if (string_compare(command, "@usercap"))
                                        {
                                            goto bad;
                                        }
                                        if (Guest(player)) { send_message(player, guest_fail()); return; };
                                        do_usercap(player, arg1);
                                        break;
                                    default:
                                        goto bad;
                                }
                                break;
                            case 'w':
                            case 'W':
                                if (strcmp(command, "@whereis") == 0)
                                {
                                    do_whereis(player, arg1);
                                    break;
                                }
                                if ( strcmp(command, "@wipeout") == 0 )
                                {
                                    if (!is_direct)
                                    {
                                        goto bad;
                                    }
                                    do_wipeout(player, arg1, arg2);
                                    break;
                                }
                                if ( strcmp(command, "@wemit") == 0 )
                                {
                                    if (Guest(player)) { send_message(player, guest_fail()); return; };
                                    do_wemit(player, arg1, arg2);
                                    break;
                                }
                                Matched("@wait");
                                if (Guest(player)) { send_message(player, guest_fail()); return; };
                                wait_que(player,atoi(arg1), arg2,cause);
                                break;
                            case 'z':
                            case 'Z':
                                switch(command[2])
                                {
                                    case 'e':
                                    case 'E':
                                        Matched("@zemit");
                                        if (Guest(player))
                                        {
                                            do_general_emit(player, arg1, pure2, 3);
                                        }
                                        else
                                        {
                                            do_general_emit(player, arg1, arg2, 3);
                                        }
                                        break;
                                    case 'l':
                                    case 'L':
                                        Matched("@zlink");
                                        if (Guest(player)) { send_message(player, guest_fail()); return; };
                                        do_zlink(player, arg1, arg2);
                                        break;
                                    default:
                                        goto bad;
	                        }
                                break;
                            default:
                                goto bad;
                        }
                        break;
                    case 'd':
                    case 'D':
                        Matched("drop");
                        do_drop(player, arg1);
                        break;
                    case 'e':
                    case 'E':
                        switch(command[1])
                        {
                            case 'X':
                            case 'x':
                                Matched("examine");
                                do_examine(player, arg1, arg2);
                                break;
                            case 'N':
                            case 'n':
                                Matched("enter");
                                do_enter(player, arg1);
                                break;
                            default:
                                goto bad;
                        }
                        break;
                    case 'f':
                    case 'F':
                        Matched("follow");
                        do_follow(player, arg1);
                        break;
                    case 'g':
                    case 'G':
                        // get, give, go, or gripe 
                        switch(command[1])
                        {
                            case 'e':
                            case 'E':
                                Matched("get");
                                do_get(player, arg1);
                                break;
                            case 'i':
                            case 'I':
                                Matched("give");
                                do_give(player, arg1, arg2);
                                break;
                            case 'o':
                            case 'O':
                                Matched("goto");
                                do_move(player, arg1);
                                break;
                            case 'r':
                            case 'R':
                                Matched("gripe");
                                do_gripe(player, arg1, arg2);
                                break;
                            default:
                                goto bad;
                        }
                        break;
                    case 'h':
                    case 'H':
                        Matched("help");
                        do_text(player, "help", arg1, NULL);
                        //do_help(player, arg1, "help", HELPINDX, HELPTEXT, NULL); 
                        break;
                    case 'i':
                    case 'I':
                        Matched("inventory");
                        do_inventory(player);
                        break;
                    case 'j':
                    case 'J':
                        Matched("join");
                        do_join(player, arg1);
                        break;
                    case 'l':
                    case 'L':
                        switch(command[1]) {
                            case 'o':
                            case 'O':
                            case '\0':		// patch allow 'l' command to do a look
                                Matched("look");
                                do_look_at(player, arg1);
                                break;
                            case 'E':
                            case 'e':
                                Matched("leave");
                                do_leave(player);
                                break;
                            default:
                                goto bad;
                        }
                        break;
                    case 'm':
                    case 'M':
                        switch(command[1])
                        {
                            case 'o':
                            case 'O':
                            switch(command[2])
                            {
                                case 'n':
                                case 'N':
                                    Matched("money");
                                    do_money(player, arg1, arg2);
                                    break;
                                case 'v':
                                case 'V':
                                    Matched("move");
                                    do_move(player, arg1);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'n':
                case 'N':
                    // news 
                    if (string_compare(command, "news"))
                    {
                        goto bad;
                    }
                    do_text(player, "news", arg1, A_ANEWS);
                    break;
                case 'p':
                case 'P':
                    switch(command[1])
                    {
                        case 'a':
                        case 'A':
                        case '\0':
                            Matched("page");
                            do_page(player, arg1, arg2);
                            break;
                        case 'o':
                        case 'O':
                            Matched("pose");
                            do_pose(player, arg1, arg2, 0);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'r':
                case 'R':
                    switch(command[1])
                    {
                        case 'e':
                        case 'E':
                            Matched("read");	// undocumented alias for look at 
                            do_look_at(player, arg1);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 's':
                case 'S':
                    // say, "score" 
                    switch(command[1])
                    {
                        case 'a':
                        case 'A':
                            Matched("say");
                            if (Guest(player))
                            {
                                do_say(player, arg1, pure2);
                            }
                            else
                            {
                                do_say(player, arg1, arg2);
                            }
                            break;
                        case 'c':
                        case 'C':
                            Matched("score");
                            do_score(player);
                            break;
                        case 'l':
                        case 'L':
                            Matched("slay");
                            do_slay(player, arg1);
                            break;
                        case 'u':
                        case 'U':
                            Matched("summon");
                            do_summon(player, arg1);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 't':
                case 'T':
                    switch(command[1])
                    {
                        case 'a':
                        case 'A':
                            Matched("take");
                            do_get(player, arg1);
                            break;
                        case 'h':
                        case 'H':
                            Matched("throw");
                            do_drop(player, arg1);
                            break;
                        case 'o':
                        case 'O':
                            Matched("to");
                            do_to(player, arg1, arg2);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'u':
                case 'U':
                    Matched("use");
                    do_use(player, arg1);
                    break;
                case 'w':
                case 'W':
                    switch(command[1])
                    {
                        case 'h':
                        case 'H':
                            switch(command[2])
                            {
                                case 'i':
                                case 'I':
                                case '\0':
                                    Matched("whisper");
                                    if (Guest(player))
                                    {
                                        do_whisper(player, arg1, pure2);
                                    }
                                    else
                                    {
                                        do_whisper(player, arg1, arg2);
                                    }
                                    break;
                                case 'o':
                                case 'O':
                                    Matched("who");
                                    dump_users(player, arg1, arg2, NULL);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case '\0':
                            do_whisper(player, arg1, arg2);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                default:
                    goto bad;
                bad:
                    // printf("debug::process_command() : went to bad section");

                // Check for comsystem aliases 
                if (is_on_channel(player, command) >= 0)
                {
                     if (buff[0] != '\0')
                     {
                          do_com(player, command, buff);
                          break;
                     }
                }

                if (!slave && test_set(player, command, arg1, arg2, is_direct))
                {
                    if (command_log)
                    {
                        fputc('.', command_log);
                        fflush(command_log);
                    }

                    return;
                }

                // try matching user defined functions before chopping 
                match = list_check(db[db[player].location].contents, player, '$', unp)
                     || list_check(db[player].contents, player, '$', unp)
                     || atr_match(db[player].location, player, '$', unp)
                     || list_check(db[db[player].location].exits, player, '$', unp);

                DOZONE(zon, player)
                {
                    match = list_check(z = zon, player, '$', unp) || match;
                }

                if (!match)
                {
                    send_message(player, "I don't understand that command.");

                    if (db[player].flags & PLAYER_NEWBIE)
                    {
                        send_message(player, "(Type 'help' to get help on most commands.)");
                    }
                }

                break; // end of 'bad:'
            } // end of command[0]
        }
    }

    { 
        int a;

        for (a = 0; a < 10; a++) 
        {
            wptr[a] = NULL;
        }
    }

    if (command_log)
    {
        fputc('.', command_log);
        fflush(command_log);
    }

}

// get a player's or object's zone 
dbref get_zone_first(dbref player)
{
    int depth = 10;
    dbref location;

    for (location = player; depth && location!=NOTHING; depth--, location = db[location].location)
    {
        if (db[location].zone == NOTHING && (Typeof(location)==TYPE_THING || Typeof(location)==TYPE_ROOM) && location != 0 && location != db[0].zone)
        {
            db[location].zone = db[0].zone;
        }

        if (location == db[0].zone)
        {
            return db[0].zone;
        }
        else if (db[location].zone != NOTHING)
        {
            return db[location].zone;
        }
    }

    return db[0].zone;
}

dbref get_zone_next(dbref player)
{
    if (db[player].zone == NOTHING && player != db[0].zone)
    {
        return db[0].zone;
    }

    return db[player].zone;
}

static int list_check(dbref thing, dbref  player, int type, char *str)
{
    int match = 0;

    while (thing != NOTHING)
    {
        // only a player can match on him/herself 
        if (((thing == player) && (Typeof(thing) != TYPE_PLAYER)) || ((thing != player) && (Typeof(thing) == TYPE_PLAYER)))
        {
            thing = db[thing].next;
        }
        else
        {
            if (atr_match(thing,player,type,str))
            {
                match = 1;
            }

            thing=db[thing].next;
        }
    }

    return(match);
}

///////////////////////////////////////////////////////////////////////////////
// atr_match()
///////////////////////////////////////////////////////////////////////////////
//    Routine to check attribute list for wild card matches of certain
// type and queue them 
///////////////////////////////////////////////////////////////////////////////
// Notes:
//     int type;  - must be symbol not affected by compress 
//     char *str; - string to match 
///////////////////////////////////////////////////////////////////////////////
// Returns: 
///////////////////////////////////////////////////////////////////////////////
static int atr_match(dbref thing, dbref player, int type, char *str)
{
    struct all_atr_list *ptr;
    int match = 0;

    for (ptr = all_attributes(thing); ptr; ptr = ptr->next)
    {
        if ((ptr->type != 0) && !(ptr->type->flags & AF_LOCK) && (*ptr->value == type))
        {
            // decode it 
            char buff[BUF_SIZE];
            char *s, *p;

            strncpy(buff, (ptr->value), BUF_SIZE);

            // search for first un escaped : 
            for (s = buff+1; *s && (*s!=':'); s++) ;
        
            if (!*s)
            {
                continue;
            }

            *s++ = 0;

            // locked attribute 
            if (*s == '/')
            {
                p = ++s;

                while (*s && (*s != '/'))
                {
                    if (*s == '[')
                    {
                        while (*s && (*s != ']'))
                        {
                            s++;
                        }
                    }

                    s++;
                }

                if (!*s)
                {
                    continue;
                }

                *s++ = '\0';

                if (!eval_boolexp(player, thing, p, get_zone_first(player)))
                {
                    continue;
                }
            }

            if (wild_match(buff+1,color(str, REMOVE)))
            {
                if (ptr->type->flags & AF_HAVEN)
                {
                    return(0);
                }
                else
                {
                    match = 1;
                }

                if (!eval_boolexp(player,thing,atr_get(thing,A_ULOCK),get_zone_first(player)))
                {
                    did_it(player,thing,A_UFAIL, NULL,A_OUFAIL, NULL,A_AUFAIL);
                }
                else
                {
                    parse_que(thing, s, player);
                }
            }
        }
    }

    return(match);
}

int Live_Player(dbref thing)
{
    return (db[thing].flags&CONNECT);
}

int Live_Puppet(dbref thing)
{
    if (db[thing].flags&PUPPET && (Typeof(thing)!=TYPE_PLAYER) && (db[thing].flags&CONNECT || db[db[thing].owner].flags&CONNECT))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

int Listener(dbref thing)
{
    //  ALIST *ptr; 
    struct all_atr_list *ptr;
    ATTR *type;
    char *s;

    if (Typeof(thing) == TYPE_PLAYER)
    {
        // players don't hear. 
        return FALSE;
    }

    for (ptr = all_attributes(thing); ptr; ptr = ptr->next)
    {
        type = ptr->type;
        s = ptr->value;

        if (!type)
        {
            continue;
        }

        if (type == A_LISTEN)
        {
            return(TRUE);
        }

        if ((*s=='!') && !(type->flags & AF_LOCK))
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

int Commer(dbref thing)
{
    //  ALIST *ptr; 
    struct all_atr_list *ptr;
    ATTR *type;
    char *s;

    for (ptr = all_attributes(thing); ptr; ptr = ptr->next)
    {
        /*    type=AL_TYPE(ptr);
              s=AL_STR(ptr);*/
        type = ptr->type;
        s = ptr->value;

        if (!type)
        {
            continue;
        }

        if (*s == '$')
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

int Hearer(dbref thing)
{
    if (Live_Player(thing) || Live_Puppet(thing) || Listener(thing))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

#undef Matched

void exit_nicely(int i)
{
    remove_muse_pid();
#ifdef MALLOCDEBUG
    mnem_writestats();
#endif
    exit(i);
}
