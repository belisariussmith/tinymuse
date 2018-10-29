///////////////////////////////////////////////////////////////////////////////
// config.c                                                        TinyMUSE
///////////////////////////////////////////////////////////////////////////////
//    Contains 
///////////////////////////////////////////////////////////////////////////////
#include "tinymuse.h"
#include "config.h"
#include "externs.h"

// MUD Name 
char *muse_name = "TinyMUSE";

// Options
int allow_exec     = FALSE;      // Allow for executables 
int allow_create   = TRUE;       // Allow players to create their own characters
int enable_lockout = TRUE;       // Lockout

// Rooms
dbref player_start  = 0;         // database obj id # of start room (players)
dbref guest_start   = 0;         // database obj id # of start room (guests)
dbref default_room  = 0;         // database obj id # of default room

// Settings
int initial_credits = 5000;      // amount new players start with
int allowance       = 500;       // credits gained per day
int number_guests   = 10;        // Maximum number of guests at a time
int max_pennies     = 10000;     // Maximum pennies a player is allowed
char *start_quota   = "20";

#ifdef ALPHA
int inet_port = 4208;
#else
int inet_port = 4201;
#endif

int max_output     = 16384;     // number of bytes until output flushed 
int max_input      = 1024;      // maximum # of bytes taken from client at a time

int command_time_msec  = 1000;  // time slice length (milliseconds)
int command_burst_size = 100;   // number of commands allowed in a burst
int commands_per_time  = 1;     // commands per slice after burst

int warning_chunk = 50;         // number of non-checked objects that should
				// be gone through when doing incremental
				// warnings
int warning_bonus = 30;		// number of non-checked objects that one
				// checked object counts as.

int fixup_interval = 311;
int dump_interval  = 1994;
int garbage_chunk  = 3;

char *bad_object_doomsday = "600";
char *default_doomsday    = "600";

// Database
char *def_db_in  = "run/db/mdb";
char *def_db_out = "run/db/mdb";

// Logfiles
char *stdout_logfile       = "run/logs/out.log";
char *wd_logfile           = "run/logs/wd.log";

char *muse_pid_file        = "run/logs/muse_pid";
char *wd_pid_file          = "run/logs/wd_pid";
 
char *create_msg_file      = "run/msgs/create.txt";
char *connect_msg_file     = "run/msgs/connect.txt";
char *welcome_msg_file     = "run/msgs/welcome.txt";
char *guest_msg_file       = "run/msgs/guest.txt";
char *register_msg_file    = "run/msgs/register.txt";
char *leave_msg_file       = "run/msgs/leave.txt";
char *guest_lockout_file   = "run/lockouts/guest-lockout";
char *welcome_lockout_file = "run/lockouts/welcome-lockout";

// root player db ID #
dbref root = 1;                  // Should match the # in the Database 

// Costs
int thing_cost    = 10;          // Generic objects
int exit_cost     = 1;           // Exits
int room_cost     = 10;          // Rooms
int robot_cost    = 1000;        // Robots/Puppets
int link_cost     = 1;           // Object Link 
int find_cost     = 10;
int search_cost   = 10;
int page_cost     = 1;           // Cost to send a page
int announce_cost = 250;         // Cost to make an announcement
int queue_cost    = 10;          // Deposit cost 
int queue_loss    = 15;          // 1/queue_loss lost for each queue command 

int max_queue     = 100;         // maximum queue commands per player 

int player_name_limit      = 32; // maximum length of player name. 
int player_reference_limit = 5;  // longest name player can be
				 // referenced by. 
int max_mail_age      = 0;       // maximum age of old, read +mail 
int old_mail_interval = 1800;    // Clear old mail every 30 minutes.

char *guest_prefix      = "Guest";
char *guest_description = "A guest player.  Please treat with respect.";

// Messages 
static char *guest_msgs[] = {
  "Sorry, guests may not do that.",
  0
};

char *BROKE_MESSAGE = "You're broke!";

char *guest_fail()
{
  static int messageno = -1;

  if (guest_msgs[++messageno] == 0)
  {
      messageno = 0;
  }

  return guest_msgs[messageno];
}

static char *perm_messages[] = {
    "Permission denied.",
    0
};

char *perm_denied()
{
    static int messageno = -1;

    if (perm_messages[++messageno] == 0)
    {
        messageno = 0;
    }

    return perm_messages[messageno];
}

// name of +com channel to which db-info announcements should be
// sent to.
char *dbinfo_chan = "dbinfo";

// name of +com channel to which connection information will be posted. 
// conn_chan = regular users' connections channel                       
// dconn_chan = directors' connections channel (gives hostnames, etc) 
char *conn_chan  = "Connections";
char *dconn_chan = "*connect";

static void donum P((dbref, int *, char *));
static void dostr P((dbref, char **, char *));
static void doref P((dbref, dbref *, char *));

void info_config(dbref player)
{
    int i;

    send_message(player, "  permission denied messages:");

    for (i=0; perm_messages[i]; i++)
    {
        send_message(player, "    %s", perm_messages[i]);
    }

// What ridiculous nonsense is this? (Belisarius)
#define DO_NUM(str,var) send_message(player, "  %-22s: %d" ,str, var);
#define DO_STR(str,var) send_message(player, "  %-22s: %s" ,str, var);
#define DO_REF(str,var) send_message(player, "  %-22s: #%d" , str, var);
#include "conf.h"
#undef DO_NUM
#undef DO_STR
#undef DO_REF

}

static void donum(dbref player, int *var, char *arg2)
{
    if (!strchr("-0123456789", *arg2))
    {
        send_message(player, "must be a number.");
        return;
    }

    (*var) = atoi(arg2);

    send_message(player, "set.");
}

static void dostr(dbref player, char **var, char *arg2)
{
    if (!*arg2)
    {
        send_message(player, "must give new string.");
        return;
    }

    SET(*var, arg2);

    send_message(player,"set.");
}

static void doref(dbref player, dbref *var, char *arg2)
{
    dbref thing;
    thing = match_thing(player, arg2);

    if (thing == NOTHING)
    {
        return;
    }

    (*var) = thing;

    send_message(player,"set.");
}

void do_config(dbref player, char *arg1, char *arg2)
{
    static int initted = 0;

    if (player != root)
    {
        send_message(player, perm_denied());
        return;
    }

    if (!initted)
    {
        char *newv;

        // we need to re-allocate strings so we can change them. 
// What ridiculous nonsense is this? (Belisarius)
#define DO_NUM(str,var) ;
#define DO_STR(str,var) newv=malloc(strlen(var)+1);strcpy(newv,var);var=newv;
#define DO_REF(str,var) ;
#include "conf.h"
#undef DO_NUM
#undef DO_STR
#undef DO_REF
        initted = 1;
    }
// What ridiculous nonsense is this? (Belisarius)
#define DO_NUM(str,var) if(!string_compare(arg1,str)) donum(player, &var, arg2); else
#define DO_STR(str,var) if(!string_compare(arg1,str)) dostr(player, &var, arg2); else
#define DO_REF(str,var) if(!string_compare(arg1,str)) doref(player, &var, arg2); else
#include "conf.h"
#undef DO_NUM
#undef DO_STR
#undef DO_REF
    // else
    send_message(player, "no such config option: %s", arg1);
}
