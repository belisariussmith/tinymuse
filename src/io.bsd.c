///////////////////////////////////////////////////////////////////////////////
// bsd.c                                                        TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: bsd.c,v 1.35 1993/12/19 17:59:49 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include "tinymuse.h"
#include "config.h"
#include "externs.h"
#include "ident.h"

#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include "interface.h"
#include <netdb.h>
#include <ctype.h>
#include <time.h> // Belisarius - fixes dereferncing pointer to incomplete type 'struct tm' error 

#define  DEF_MODE       0644

#define  WHO_BUF_SIZ    300
#define  DEF_SCR_COLS   78        // must be less than WHO_BUF_SIZ
#define  MIN_SEC_SPC    2
#define  MIN_GRP_SPC    4

#define  DEF_WHO_FLAGS  "Nafoicd" // close to the old WHO output format
#define  DEF_WHO_ALIAS  ""        // what people see if no alias is set

#define  W_NAME         0x001
#define  W_ALIAS        0x002
#define  W_FLAGS        0x004
#define  W_ONFOR        0x008
#define  W_IDLE         0x010
#define  W_CONCID       0x020
#define  W_HOST         0x040
#define  W_PORT         0x080
#define  W_DOING        0x100
#define  W_CLASS        0x200

#define  mklinebuf(fp) setlinebuf(fp)

time_t now;

static char who_flags[] = "nafoixhpdc";

static char *who_fmt_small[] = {
  "%-10.10s ", "%-6.6s ", "%-5.5s ",
  "%9.9s ", "%4.4s ", "%-5.5s ", "%-20.20s ", "%-6s ", "%-15.15s ",
  "%-5.5s "
};

static int  who_sizes_small[] = { 10, 6, 5, 9, 4, 5, 20, 6, 15, 5 };

static char *who_fmt_large[] = {
  "%-16.16s ", "%-16.16s ", "%-5.5s ", "%9.9s ", "%4.4s ", "%-13.13s ",
  "%-32.32s ", "%-6s ", "%-40.40s ", "%-11.11s " };
  static int  who_sizes_large[] = { 16, 16, 5, 9, 4, 13, 32, 6, 40, 11 };

char who_bh_buf[33];

#define  WHO_SIZE    10
static int who_sizes[WHO_SIZE];
static char *who_fmt[WHO_SIZE];

#define MALLOC(result, type, number) do {            \
                                  if (!((result) = (type *) malloc ((number) * sizeof (type))))    \
                                    panic("Out of memory");                \
                                    } while (0)

#define FREE(x) (free((void *) x))

extern int errno;
extern int reserved;
static void init_args P((int, char **));
static void init_io P((void));
static void set_signals P((void));
static void save_muse_pid P((void));
static void open_sockets P((void));
static void mud_loop P((int));
static void close_sockets P((void));
static char *wholev P((dbref, int, int));

fd_set input_set, output_set;

int shutdown_flag = FALSE;
int exit_status   = 136;    // we will change this when we set shutdown_flag to 1 (true)
int signal_caught = 0;

int need_more_proc;
int maxd = 0;

time_t muse_up_time;
time_t next_dump;
time_t next_mail_clear;

static char *connect_fail = "Either that player does not exist, or has a different password.\n";
#ifndef WCREAT
static char *create_fail = "Either there is already a player with that name, or that name is illegal.\n";
#endif
static char *flushed_message = "<Output Flushed>\n";
static char *shutdown_message = "MUSE rebooting... please hold.\n";
static char *usercap_message = "Sorry, the MUSE is already at maximum capacity. Please try again later.\n";
static char *lockout_message = "Sorry, the MUSE is currently under restricted access conditions.\nPlease try again later.\n";
static char *nologins_message = "No logins are being allowed at this time. Please try again later.\n";

static char *get_password = "Please enter password:\n\373\001";
static char *got_password = "\374\001";

struct descriptor_data *descriptor_list = 0;

static int sock = -1;
int ndescriptors = 0;
char ccom[BUF_SIZE];
dbref cplr;

int restrict_connect_class = 0;
int user_limit = 0;
int user_count = 0;
int nologins = 0;

static void check_connect P((struct descriptor_data *, char *));
static void parse_connect  P((char *, char *, char *, char *));
static void sig_handler P((int));
static void set_userstring  P((char **, char *));
static int do_command  P((struct descriptor_data *, char *));
static char *strsave  P((char *));
static signal_type dump_status P((int));
static signal_type do_sig_reboot P((int));
static struct descriptor_data *new_connection P(());
static void clearstrings P((struct descriptor_data *));
static void make_nonblocking P((int));
static int make_socket P((int));
static struct text_block *make_text_block P((char *, int));
//static void add_to_queue P((struct text_queue *, char *, int));
static void add_to_queue(struct text_queue *q, char *b, int n);
//static int flush_queue P((struct text_queue *, int));
static int flush_queue(struct text_queue *q, int n);
static int queue_write P((struct descriptor_data *,char *, int));
static struct descriptor_data *initializesock P((int, struct sockaddr_in *, char *, enum descriptor_state));
static int check_lockout P((struct descriptor_data *, char *, char *));
static int process_output P((struct descriptor_data *));
static void freeqs P((struct descriptor_data *));
static void connect_message P((struct descriptor_data *,char *,int));
static void process_commands P((void));
static char *make_guest P((struct descriptor_data *));
static signal_type bailout P((int));
static int process_input P((struct descriptor_data *));

int main(int argc, char *argv[])
{
    char buf[256];

    // Initialization 
    init_args(argc, argv);
    init_io();
    printf("-----------------------------------------------\n");
    printf("MUSE online (pid=%d)\n", getpid());
    init_attributes();
    init_mail();
    init_timer();
    open_sockets();

    // Check database 
    if ( init_game(def_db_in, def_db_out) < 0 )
    {
        // Looks like there was a problem loading the DB,
        // so there's no point in continuing.
        sprintf(buf, "Couldn't load %s!", def_db_in);
        log_error(buf);
        exit_nicely(136);
    }

    set_signals();

    save_muse_pid();

    time(&muse_up_time);

    now = time(NULL);
    next_dump = now + dump_interval;
    next_mail_clear = now + old_mail_interval;

    // Start the Game Loop 
    mud_loop(inet_port);

    //
    // Game loop has completed, so let's shut
    // everything down gracefully.
    //
    log_important("Shutting down normally.");
    close_sockets();
    do_haltall(1);
    dump_database();
    free_database();
    free_mail();
    free_hash();

    //
    // If this is a reboot, then we can just "reset" the
    // socket by setting the "file" descriptor back to 1.
    //
    if (exit_status == STATUS_REBOOT)
    {
        fcntl(sock, F_SETFD, TRUE);
    }
    else
    {
        //
        // This is not a reboot, we are definitely shutting
        // down, so we close the the socket.
        //
        close(sock);
    }

    if ( signal_caught > 0 )
    {
        sprintf(buf, "Shutting down due to signal %d", signal_caught);
        log_important(buf);
    }

    if (exit_status == STATUS_REBOOT)
    {
        // Let's reboot 
        remove_muse_pid();
        close_logs();
#ifdef MALLOCDEBUG
        mnem_writestats();
#endif
        if (fork() == FALSE)
        {
            exit(0); // safe exit stuff, like profiling data, stdio flushing. 
                  
        }

        wait(0);
        execv(argv[0],argv);
        execv("bin/tinymuse",argv);
        execvp("tinymuse", argv);
        unlink("run/logs/socket_table");
        save_muse_pid();
        _exit(exit_status);
    }

    exit_nicely(exit_status);
}

static void save_muse_pid()
{
    FILE *fp;
    struct stat statbuf;

    if (stat(muse_pid_file, &statbuf) >= 0)
    {
        // it already exists! 
        log_error("found muse_pid. Is server already running? crashed? If crashed, recommend muse_pid be deleted");
        exit_nicely(136);
    }

    unlink(muse_pid_file);

    if ( (fp = fopen(muse_pid_file, "w")) != NULL )
    {
        fprintf(fp, "%d\n", getpid());
        fclose(fp);
    }
}

void remove_muse_pid()
{
    FILE *fp;
    char buf1[200], buf2[200];

    close(reserved);

    if (!(fp = fopen(muse_pid_file, "r")))
    {
        perror("muse pid file");
        log_error("can't open muse pid file!");
        return;
    }

    fgets(buf1, 20, fp);
    sprintf(buf2, "%d\n", getpid());

    if (strcmp(buf1, buf2))
    {
        sprintf(buf1, "muse pid file doesn't contain me(%d)!", getpid());
        log_error(buf1);
        fclose(fp);

        return;
    }

    fclose(fp);
    unlink(muse_pid_file);

    reserved = open("/dev/null", O_RDWR, 0);
}

static void init_args(int argc, char *argv[])
{
    // change default input database? 
    if ( argc > 1 )
    {
        --argc, def_db_in = *++argv;
    }

    // change default dump database? 
    if ( argc > 1 )
    {
        --argc, def_db_out = *++argv;
    }

    // change default log file? 
    if ( argc > 1 )
    {
        --argc, stdout_logfile = *++argv;
    }

    // change port number? 
    if ( argc > 1 )
    {
        --argc, inet_port = atoi(*++argv);
    }
}

static void init_io()
{
    char buf[BUF_SIZE];
    extern int fileno P((FILE *));
    int fd;

    // close standard input
    // fclose(stdin);

    // open a link to the log file 
    fd = open(stdout_logfile, O_WRONLY | O_CREAT | O_APPEND, DEF_MODE);
    if ( fd < 0 )
    {
        perror("open()");
        sprintf(buf, "error opening %s for writing", stdout_logfile);
        log_error(buf);
        exit_nicely(136);
    }

    // attempt to convert standard output to logfile
    close(fileno(stdout));
    if ( dup2(fd, fileno(stdout)) == -1 )  {
        perror("dup2()");
        log_error("error converting standard output to logfile");
    }
    mklinebuf(stdout);

    // attempt to convert standard error to logfile
    close(fileno(stderr));
    if ( dup2(fd, fileno(stderr)) == -1 )  {
        perror("dup2()");
        printf("error converting standard error to logfile\n");
    }
    mklinebuf(stderr);

    // this logfile reference is no longer needed
    close(fd);

    // save a file descriptor 
    reserved = open("/dev/null",O_RDWR, 0);
}

static void set_signals()
{
    // we don't care about SIGPIPE, we notice it in select() and write() 
    signal(SIGPIPE, SIG_IGN);
    signal (SIGCLD, SIG_IGN);

    // standard termination signals 
    signal (SIGINT, bailout);

    // catch these because we might as well 
    /*signal (SIGQUIT, bailout);
      signal (SIGILL, bailout);
      signal (SIGTRAP, bailout);
      signal (SIGIOT, bailout);
      signal (SIGEMT, bailout);
      signal (SIGFPE, bailout);
      signal (SIGBUS, bailout);
      signal (SIGSEGV, bailout);
      signal (SIGSYS, bailout);
      signal (SIGTERM, bailout);
      signal (SIGXCPU, bailout);
      signal (SIGXFSZ, bailout);
      signal (SIGVTALRM, bailout);
      signal (SIGUSR2, bailout);
      * want a core dump for now!! */

    // status dumper (predates "WHO" command) 
    signal(SIGUSR1, dump_status);

    // signal to reboot the MUSE 
    signal(SIGUSR2, do_sig_reboot);
    signal (SIGHUP, sig_handler);
    signal (SIGTERM, sig_handler);
}

/////////////////////////////////////////////////////////////////////
// sig_handler()
/////////////////////////////////////////////////////////////////////
//    This is called by signal() when a signal that requires the MUD
// to shutdown (or restart) is sent to the server.
//
//    The MUD will attempt to shutdown gracefully no matter what
// signal is sent, with the exception of a SIGHUP that is meant to
// be interpreted as a restart signal.
/////////////////////////////////////////////////////////////////////
// Returns: -
/////////////////////////////////////////////////////////////////////
static void sig_handler(int signal)
{
    // Capture the signal that was caught.
    signal_caught = signal;

    // Check signal type
    if (signal == SIGTERM)
    {
        // No special exit status required 
        exit_status = STATUS_NONE;
    }
    else if (signal == SIGHUP)
    {
        // SIGHUP, so let's restart instead of terminating 
        exit_status = STATUS_REBOOT;
    }
    //
    // Different signals may require a special exit status, and
    // those should be added below.
    //

    // Signal the shutdown
    shutdown_flag = TRUE;
}

char *short_name(dbref obj)
{
    int a,b;

    if (obj < 0 || obj >= db_top)
    {
        return "?";
    }

    a = strlen(atr_get(obj, A_ALIAS));
    b = strlen(db[obj].name);

    if (a && a < b)
    {
        return atr_get(obj, A_ALIAS);
    }
    else
    {
        return(db[obj].name);
    }
}

///////////////////////////////////////////////////////////////////////////////
// message_ch_raw()
///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
// Notes:
//    Intended to be a replacement for raw_notify() 
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
void message_ch_raw(dbref player, char *msg)
{
    struct descriptor_data *d;
    extern dbref speaker, as_from, as_to;
    char newmsg[MAX_CMD_BUF];

    if (!(db[player].flags & CONNECT) && player != as_from)
    {
        return;
    }

    if (IS(player,TYPE_PLAYER,PUPPET))
    {
        static char buf[BUF_SIZE*2];
        strcpy(buf, msg);
        if (speaker != player)
        {
            sprintf(buf+strlen(buf), " |NORMAL|[#%d/%s]", speaker, short_name(db[speaker].owner));
        }
        msg = buf;
    }

    if (player == as_from)
    {
        player = as_to;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->state == CONNECTED && d->player == player)
        {
            if (Typeof(player)==TYPE_PLAYER && db[player].flags & PLAYER_ANSI)
            {
                sprintf(newmsg, "%s|NORMAL|", msg);
                queue_string(d, color(newmsg, ADD));
            }
            else
            {
                queue_string(d, color(msg, REMOVE));
            }

            queue_write(d, "\n", 1);
        }
    }
}

static struct timeval timeval_sub(struct timeval now, struct timeval then)
{
    now.tv_sec -= then.tv_sec;
    now.tv_usec -= then.tv_usec;

    while (now.tv_usec < 0)
    {
        now.tv_usec += 1000000;
        now.tv_sec--;
    }

    // aack! 
    if (now.tv_sec < 0)
    {
        now.tv_sec = 0;
    }

    return now;
}

static int msec_diff(struct timeval now, struct timeval then)
{
    return ((now.tv_sec - then.tv_sec) * 1000 + (now.tv_usec - then.tv_usec) / 1000);
}

static struct timeval msec_add(struct timeval t, int x)
{
    t.tv_sec += x / 1000;
    t.tv_usec += (x % 1000) * 1000;

    if (t.tv_usec >= 1000000)
    {
        t.tv_sec += t.tv_usec / 1000000;
        t.tv_usec = t.tv_usec % 1000000;
    }

    return t;
}

static struct timeval update_quotas(struct timeval last, struct timeval current)
{
    int nslices;
    struct descriptor_data *d;

    nslices = msec_diff (current, last) / command_time_msec;

    if (nslices > 0)
    {
        for (d = descriptor_list; d; d = d -> next)
        {
            d -> quota += commands_per_time * nslices;

            if (d -> quota > command_burst_size)
            {
                d -> quota = command_burst_size;
            }
        }
    }

    return msec_add (last, nslices * command_time_msec);
}

static void mud_loop(int port)
{
    char buf[256];
    struct timeval last_slice, current_time;
    struct timeval next_slice;
    struct timeval timeout, slice_timeout;
    int found;
    struct descriptor_data *d, *dnext;
    struct descriptor_data *newd;
    int avail_descriptors;

    sprintf(buf, "Starting up on port %d", port);
    log_io(buf);

    sock = make_socket (port);

    if (maxd <= sock)
    {
        maxd = sock+1;
    }

    gettimeofday(&last_slice, (struct timezone *) 0);

    avail_descriptors = getdtablesize() - 5;

    // Primary part of the server loop
    while ( (shutdown_flag == FALSE) && (!db_loaded) )
    {
        load_database();

        printf("debug:: passed load_database() \n");

        FD_ZERO (&input_set);
        FD_ZERO (&output_set);

        if (ndescriptors < avail_descriptors && sock >= 0)
        {
            FD_SET (sock, &input_set);
        }

        for (d = descriptor_list; d; d=dnext)
        {
            dnext = d->next;

            if (d->cstatus&C_REMOTE && d->output.head)
            {
                if (!process_output(d))
                {
                    shutdownsock(d);
                }
                need_more_proc = 1;
            }
        }

        for (d = descriptor_list; d; d=d->next)
        {
            if (!(d->cstatus & C_REMOTE))
            {
                if (d->output.head && (d->state != CONNECTED || d->player > 0))
                {
                    FD_SET (d->descriptor, &output_set);
                }
            }
        }

        for (d = descriptor_list; d; d = dnext)
        {
            dnext = d->next;

            if (d->cstatus & C_REMOTE)
            {
                process_output(d);
            }
        }

        for (d = descriptor_list; d; d = dnext)
        {
            dnext = d->next;

            if ( !(d->cstatus & C_REMOTE)
                   && (FD_ISSET (d->descriptor, &output_set)))
            {
                if (!process_output(d))
                {
                    shutdownsock(d);
                }
            }
        }
    }

    printf("debug:: everything ready and database is loaded \n");

    if (shutdown_flag == FALSE)
    {
        printf("debug:: shutdown_flag turned off (good!) \n");
    }

    while (shutdown_flag == FALSE)
    {
        printf("debug:: >> game loop << \n");
        gettimeofday(&current_time, (struct timezone *) 0);
        last_slice = update_quotas (last_slice, current_time);

        printf("debug:: processing commands \n");
        process_commands();

        if (shutdown_flag)
        {
            break;
        }

        printf("debug:: running dispatch \n");
        // test for events 
        dispatch();

        // any queued robot commands waiting? 
        timeout.tv_sec  = (need_more_proc||test_top()) ? 0 : 100;
        need_more_proc  = 0;
        timeout.tv_usec = 5;
        next_slice      = msec_add (last_slice, command_time_msec);
        slice_timeout   = timeval_sub (next_slice, current_time);

        FD_ZERO (&input_set);
        FD_ZERO (&output_set);

        printf("debug:: handling descriptors \n");
        if (ndescriptors < avail_descriptors && sock >= 0)
        {
            FD_SET (sock, &input_set);
        }

        for (d = descriptor_list; d; d=dnext)
        {
            dnext = d->next;
            if (d->cstatus & C_REMOTE && d->output.head)
            {
                if (!process_output(d))
                {
                    shutdownsock(d);
                }
                need_more_proc = 1;
             }
        }

        for (d = descriptor_list; d; d=d->next)
        {
            if(!(d->cstatus&C_REMOTE))
            {
                if (d->input.head)
                {
                    timeout = slice_timeout;
                }
                else
                {
                    FD_SET (d->descriptor, &input_set);
                }

                if (d->output.head && (d->state!=CONNECTED || d->player>0))
                {
                    FD_SET (d->descriptor, &output_set);
                }
            }
        }

        if ( (found=select (maxd, &input_set, &output_set,
               (fd_set *) 0, &timeout)) < 0)
        {
            if (errno != EINTR)
            {
                perror ("select");
                // return; // naw.. stay up. 
            }
        }
        else
        {
            // if !found then time for robot commands 
            time(&now);

            if (db_loaded && !found)
            {
                if(do_top() && do_top())
                {
                    do_top();
                }
                continue;
            }

            (void) time (&now);

            if ( (sock >= 0) && (FD_ISSET (sock, &input_set)) )
            {
                if (!(newd = new_connection (sock)))
                {
                    if (errno
                        && errno != EINTR
                        && errno != EMFILE
                        && errno != ENFILE)
                    {
                        perror ("new_connection");
                        // return; // naw.. stay up. 
                    }
                }
                else
                {
                    if (newd->descriptor >= maxd)
                    {
                        maxd = newd->descriptor + 1;
                    }
                }
            }

            for (d = descriptor_list; d; d = dnext)
            {
                dnext = d->next;

                if (FD_ISSET (d->descriptor, &input_set) && !(d->cstatus&C_REMOTE))
                {
                    if (!process_input (d))
                    {
                        shutdownsock(d);
                    }
                }
            }

            for (d = descriptor_list; d; d = dnext)
            {
                dnext = d->next;
                if (d->cstatus & C_REMOTE)
                    process_output(d);
            }

            for (d = descriptor_list; d; d = dnext)
            {
                dnext = d->next;

                if (!(d->cstatus & C_REMOTE)
                  && (FD_ISSET (d->descriptor, &output_set)))
                {
                    if (!process_output(d))
                    {
                        shutdownsock(d);
                    }
                }
            }

            for (d = descriptor_list; d; d = dnext)
            {
                dnext = d->next;
                if ( (d->cstatus & C_REMOTE) && (!d->parent) )
                {
                    shutdownsock(d);
                }
            }
        }
    }
}

static struct descriptor_data* new_connection(int sock)
{
    struct sockaddr_in addr;

    int newsock;
    int addr_len;

    char *ct;

    addr_len = sizeof (addr);
    newsock = accept (sock, (struct sockaddr *) & addr, (socklen_t *)&addr_len);

    if (newsock <= 0)
    {
        if (errno == EALREADY)
        {
            // screwy sysv buf. 
            static int k = 0;

            if (k++ > 50)
            {
                log_error("Killin' EALREADY. restartin' socket.");
                puts("Killin' EALREADY. restartin' socket.");
                close(sock);
                sock = make_socket(4201);
                k = 0;
            }
        }
        return 0;
    }
    else
    {
        struct hostent *hent;
        char buff[100];
        time_t tt;

        hent = gethostbyaddr ((char *)&(addr.sin_addr.s_addr), sizeof (addr.sin_addr.s_addr), AF_INET);

        if (hent)
        {
            strcpy (buff, hent->h_name);
        }
        else
        {
            extern char *inet_ntoa();
            strcpy(buff, inet_ntoa(addr.sin_addr));
        }
        tt = now;
        ct = ctime(&tt);

        if (ct && *ct)
        {
            ct[strlen(ct)-1] = '\0';
        }

        return initializesock (newsock, &addr,buff, WAITCONNECT);
    }
}

static void clearstrings(struct descriptor_data *d)
{
    if (d->output_prefix)
    {
        FREE(d->output_prefix);
        d->output_prefix = 0;
    }

    if (d->output_suffix)
    {
        FREE(d->output_suffix);
        d->output_suffix = 0;
    }
}

void shutdownsock(struct descriptor_data *d)
{
    char buf[512];
    int count;
    dbref guest_player;
    struct descriptor_data *sd;

    // if this is a guest player, save his reference # 
    guest_player = NOTHING;

    if (d->state == CONNECTED)
    {
        if (d->player > 0)
        {
            if (Guest(d->player))
            {
                guest_player = d->player;
            }
        }
    }

    if (d->state == CONNECTED)
    {
        if (d->player > 0)
        {
            sprintf(buf, "DISCONNECT concid %d player %s(%d)", d->concid, db[d->player].name, d->player);
            log_io (buf);
            if (!(*atr_get(d->player,A_LHIDE)))
            {
                sprintf(buf, "[%s] * DISCONNECT: %s(#%d)", conn_chan, db[d->player].name,d->player);
                com_send(conn_chan, buf);
            }

            sprintf(buf, "[%s] * DISCONNECT: %s(#%d) (concid %d)", dconn_chan, db[d->player].name, d->player, d->concid);
            com_send(dconn_chan, buf);

            announce_disconnect(d->player);
            user_count--;
        }
    }
    else
    {
        log_io (buf);
        sprintf(buf, "DISCONNECT concid %d never connected", d->concid);
        user_count--;
    }

    FD_CLR (d->descriptor, &input_set);
    FD_CLR (d->descriptor, &output_set);

    clearstrings(d);

    if (!(d->cstatus & C_REMOTE))
    {
        shutdown(d->descriptor, 0);
        close(d->descriptor);
    }
    else
    {
        register struct descriptor_data *k;

        for (k = descriptor_list; k; k = k->next)
        {
            if (k->parent == d)
            {
                k->parent = 0;
            }
        }
    }

    freeqs(d);
    *d->prev = d->next;

    if (d->next)
    {
        d->next->prev = d->prev;
    }

    if(!(d->cstatus&C_REMOTE))
    {
        ndescriptors--;
    }

    FREE(d);

    // if this is a guest account and the
    // last to disconnect from it, kill it 
    if ( guest_player != NOTHING )
    {
        count = 0;

        for (sd = descriptor_list; sd; sd = sd->next)
        {
            if (sd->state == CONNECTED && sd->player == guest_player)
            {
                ++count;
            }
        }

        if ( count == 0 )
        {
            destroy_guest(guest_player);
        }
    }
}

void outgoing_setupfd(dbref player, int fd)
{
    struct descriptor_data *d;

    ndescriptors++;
    MALLOC(d, struct descriptor_data, 1);
    d->descriptor = fd;
    d->concid = make_concid();
    d->cstatus = 0;
    d->parent = 0;
    d->state = CONNECTED;
    make_nonblocking(fd);
    d->player = -player;
    d->output_prefix=0;
    d->output_suffix=0;
    d->output_size=0;
    d->output.head=0;
    d->output.tail = &d->output.head;
    d->input.head = 0;

    d->input.tail = &d->input.head;
    d->raw_input = 0;
    d->raw_input_at = 0;
    d->quota=command_burst_size;
    d->last_time=0;
    //  d->address=inet_addr("127.0.0.1"); * localhost. cuz. 
    strcpy(d->addr,"RWHO");

    if (descriptor_list)
    {
        descriptor_list->prev = &d->next;
    }

    d->next = descriptor_list;
    d->prev = &descriptor_list;
    descriptor_list = d;

    if (fd >= maxd)
    {
        maxd = fd + 1;
    }
}

static struct descriptor_data *initializesock(int s, struct sockaddr_in *a, char *addr, enum descriptor_state state)
{
    char buf[256];
    struct descriptor_data *d;
    char *ct;
    time_t tt;

    ndescriptors++;
    MALLOC(d, struct descriptor_data, 1);
    d->descriptor    = s;
    d->concid        = make_concid();
    d->cstatus       = 0;
    d->parent        = 0;
    d->state         = state;
    make_nonblocking (s);
    d->output_prefix = 0;
    d->output_suffix = 0;
    d->output_size   = 0;
    d->output.head   = 0;
    d->output.tail   = &d->output.head;
    d->input.head    = 0;
    d->input.tail    = &d->input.head;
    d->raw_input     = 0;
    d->raw_input_at  = 0;
    d->quota         = command_burst_size;
    d->last_time     = 0;
    strncpy(d->addr, addr, 50);
    d->address       = *a;            // added 5/3/90 SCG 
    if (descriptor_list)
    {
        descriptor_list->prev = &d->next;
    }
    d->next          = descriptor_list;
    d->prev          = &descriptor_list;
    d->user          = ident_id(s, 20);
    descriptor_list  = d;

    tt = now;
    ct = ctime(&tt);
    ct[strlen(ct) - 1] = 0;

    sprintf(buf, "USER CONNECT: concid: %d host %s@%s time: %s", d->concid, d->user, d->addr, ct);
    log_io(buf);
        
    if (state == WAITCONNECT)
    {
        if (check_lockout(d, welcome_lockout_file, welcome_msg_file))
        {
            process_output (d);
            shutdownsock(d);
        }
    }

    if (user_count > user_limit)
    {
        sprintf(buf, "Refused connection from des %d due to @usercap.", d->descriptor);
        log_io(buf);

        write (d->descriptor, usercap_message, strlen (usercap_message));

        sprintf(buf, "%d %d", user_count, user_limit);
        write (d->descriptor, buf, strlen (buf));

        process_output(d);
        shutdownsock(d);

        return 0;
    }

    if (nologins)
    {
        sprintf(buf, "Refused connection from des %d due to @nologins.", d->descriptor);
        log_io(buf);
        write (d->descriptor, nologins_message, strlen (nologins_message));
        process_output(d);
        shutdownsock(d);

        return 0;
    }

    if (d->descriptor >= maxd)
    {
        maxd = d->descriptor+1;
    }     

    return d;
}

static int make_socket(int port)
{
    char buf[BUF_SIZE];
    int s;
    struct sockaddr_in server;
    int opt;

    s = socket (AF_INET, SOCK_STREAM, 0);

    if (s < 0)
    {
        sprintf(buf, "creating stream socket on port %d", port);
        perror (buf);

        exit_status   = TRUE; // try again. 
        shutdown_flag = TRUE;

        return -1;
    }

    opt = 1;

    if (setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof (opt)) < 0)
    {
        perror ("setsockopt");

        shutdown_flag = TRUE;
        exit_status   = TRUE;

        close(s);

        return -1;
    }

    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port        = htons (port);

    if (bind (s, (struct sockaddr *) & server, sizeof (server)))
    {
        perror ("binding stream socket");
        close (s);

        shutdown_flag = TRUE;
        exit_status   = TRUE;

        return -1;
    }

    listen (s, 5);

    return s;
}

static struct text_block *make_text_block(char *s, int n)
{
    struct text_block *p;

    MALLOC(p, struct text_block, 1);
    MALLOC(p->buf, char, n);
    bcopy (s, p->buf, n);
    p->nchars = n;
    p->start = p->buf;
    p->nxt = 0;

    return p;
}

static void free_text_block (struct text_block *t)
{
    FREE (t->buf);
    FREE ((char *) t);
}

static void add_to_queue(struct text_queue *q, char *b, int n)
{
    struct text_block *p;

    if (n == 0)
    {
        return;
    }

    p = make_text_block (b, n);
    p->nxt = 0;
    *q->tail = p;
    q->tail = &p->nxt;
}

static int flush_queue(struct text_queue *q, int n)
{
    struct text_block *p;
    int really_flushed = 0;

    n += strlen(flushed_message);

    while (n > 0 && (p = q->head))
    {
        n -= p->nchars;
        really_flushed += p->nchars;
        q->head = p->nxt;
        free_text_block (p);
    }

    p = make_text_block(flushed_message, strlen(flushed_message));
    p->nxt = q->head;
    q->head = p;

    if (!p->nxt)
    {
        q->tail = &p->nxt;
    }

    really_flushed -= p->nchars;

    return really_flushed;
}

static int queue_write(struct descriptor_data *d, char *b, int n)
{
    int space;

    if (d->cstatus & C_REMOTE)
    {
        need_more_proc = 1;
    }

    space = max_output - d->output_size - n;

    if (space < 0)
    {
        d->output_size -= flush_queue(&d->output, -space);
    }

    add_to_queue (&d->output, b, n);

    d->output_size += n;

    return n;
}

int queue_string(struct descriptor_data *d, char *s)
{
    return queue_write (d, s, strlen (s));
}

static int process_output(struct descriptor_data *d)
{
    struct text_block **qp, *cur;
    int cnt;

    if (d->cstatus & C_REMOTE)
    {
        static char buf[10];
        static char obuf[BUF_SIZE*2];
        int buflen;
        int k,j;
        sprintf(buf, "%d ", d->concid);
        buflen = strlen(buf);

        bcopy(buf, obuf, buflen);
        j = buflen;

        for (qp = &d->output.head; (cur= *qp);)
        {
            need_more_proc = 1;

            for(k = 0; k < cur->nchars; k++)
            {
                obuf[j++] = cur->start[k];

                if (cur->start[k] == '\n')
                {
                    if (d->parent)
                    {
                        queue_write(d->parent, obuf, j);
                    }

                    bcopy(buf, obuf, buflen);

                    j = buflen;
                }
            }

            d->output_size -= cur->nchars;

            if(!cur->nxt)
            {
                d->output.tail = qp;
            }

            *qp = cur->nxt;
            free_text_block(cur);
        }

        if (j > buflen)
        {
            queue_write(d, obuf+buflen, j-buflen);
        }
        return TRUE;
    }
    else
    {
        for (qp = &d->output.head; (cur = *qp);)
        {
            cnt = write (d->descriptor, cur -> start, cur -> nchars);

            if (cnt < 0)
            {
                if (errno == EWOULDBLOCK)
                {
                    return TRUE;
                }

                return FALSE;
            }

            d->output_size -= cnt;

            if (cnt == cur -> nchars)
            {
                if (!cur -> nxt)
                    d->output.tail = qp;

                *qp = cur -> nxt;
                free_text_block (cur);

                // do not adv ptr 
                continue;
            }

            cur -> nchars -= cnt;
            cur -> start += cnt;
            break;
        }
    }

    return TRUE;
}

static void make_nonblocking(int s)
{
    if (fcntl (s, F_SETFL, FNDELAY) == -1)
    {
        perror ("make_nonblocking: fcntl");
        panic ("FNDELAY fcntl failed");
    }
}

static void freeqs(struct descriptor_data *d)
{
    struct text_block *cur, *next;

    cur = d->output.head;

    while (cur)
    {
        next = cur -> nxt;
        free_text_block (cur);
        cur = next;
    }

    d->output.head = 0;
    d->output.tail = &d->output.head;

    cur = d->input.head;

    while (cur)
    {
        next = cur -> nxt;
        free_text_block (cur);
        cur = next;
    }

    d->input.head = 0;
    d->input.tail = &d->input.head;

    if (d->raw_input)
    {
        FREE (d->raw_input);
    }

    d->raw_input = 0;
    d->raw_input_at = 0;
}

void welcome_user(struct descriptor_data *d)
{
    connect_message(d, welcome_msg_file, 0);
}

static void connect_message(struct descriptor_data *d, char *filename, int direct)
{
    int n, fd;
    char buf[BUF_SIZE];

    close(reserved);

    if ( (fd = open(filename, O_RDONLY, 0)) != -1 )
    {
        while ( (n = read(fd, buf, 512)) > 0 )
        {
            queue_write(d, buf, n);
        }

        close(fd);
        queue_write(d, "\n", 1);
    }

    reserved=open("/dev/null", O_RDWR, 0);

    if (direct)
    {
        process_output(d);
    }
}

static char *strsave (char *s)
{
    char *p;

    MALLOC (p, char, strlen(s) + 1);

    if (p)
    {
        strcpy (p, s);
    }

    return p;
}

static void save_command (struct descriptor_data *d, char *command)
{
    add_to_queue (&d->input, command, strlen(command)+1);
}

static int process_input (struct descriptor_data *d)
{
    char buf[BUF_SIZE];
    int got;
    char *p, *pend, *q, *qend;

    got = read (d->descriptor, buf, sizeof buf);

    if (got <= 0)
    {
        return FALSE;
    }

    if (!d->raw_input)
    {
        MALLOC(d->raw_input,char,MAX_COMMAND_LEN);
        d->raw_input_at = d->raw_input;
    }

    p = d->raw_input_at;
    pend = d->raw_input + MAX_COMMAND_LEN - 1;

    for (q=buf, qend = buf + got; q < qend; q++)
    {
        if (*q == '\n')
        {
            *p = '\0';
            if (p >= d->raw_input)
                save_command (d, d->raw_input);
            p = d->raw_input;
        }
        else if (p < pend && isascii (*q) && isprint (*q))
        {
            *p++ = *q;
        }
    }

    if (p > d->raw_input)
    {
        d->raw_input_at = p;
    }
    else
    {
        FREE(d->raw_input);
        d->raw_input = 0;
        d->raw_input_at = 0;
    }

    return TRUE;
}

static void set_userstring (char **userstring, char *command)
{
    if (*userstring)
    {
        FREE(*userstring);
        *userstring = 0;
    }

    while (*command && isascii (*command) && isspace (*command))
    {
        command++;
    }

    if (*command)
    {
        *userstring = strsave (command);
    }
}

static void process_commands()
{
    int nprocessed;
    struct descriptor_data *d, *dnext;
    struct text_block *t;

    do
    {
        nprocessed = 0;

        for (d = descriptor_list; d; d = dnext)
        {
            dnext = d->next;

            if (d -> quota > 0 && (t = d -> input.head))
            {
                nprocessed++;

                if (!do_command(d, t -> start))
                {
                    connect_message(d,leave_msg_file,1);
                    shutdownsock(d);
                }
                else
                {
                    d -> input.head = t -> nxt;

                    if (!d -> input.head)
                    {
                        d -> input.tail = &d -> input.head;
                    }

                    free_text_block (t);
                }
            }
        }
    } while (nprocessed > 0);
}

static int do_command (struct descriptor_data *d, char *command)
{
    d->last_time = now;
    d->quota--;
    depth = 0;

    if (!*command && !(d->player<0 && d->state == CONNECTED))
    {
        return FALSE;
    }

  /* if (d->state == CONNECTED && d->player > 0) {
    * pop player to top of who list. *
    *d->prev = d->next;
    if (d->next)
      d->next->prev = d->prev;

    d->next = descriptor_list;
    if (descriptor_list)
      descriptor_list->prev = &d->next;
    descriptor_list = d;
    d->prev = &descriptor_list;
  } */

    if (!strcmp (command, QUIT_COMMAND))
    {
        return FALSE;
    }
    else if (!strncmp (command, "I wanna be a concentrator... my password is ", sizeof ("I wanna be a concentrator... my password is ") -1))
    {
        do_becomeconc(d,command+sizeof("I wanna be a concentrator... my password is ")-1);
    }
    else if (!strncmp (command, PREFIX_COMMAND, strlen (PREFIX_COMMAND)))
    {
        set_userstring (&d->output_prefix, command+strlen(PREFIX_COMMAND));
    }
    else if (!strncmp (command, SUFFIX_COMMAND, strlen (SUFFIX_COMMAND)))
    {
        set_userstring (&d->output_suffix, command+strlen(SUFFIX_COMMAND));
    }
    else
    {
        if (d->cstatus & C_CCONTROL)
        {
            if (!strcmp(command, "Gimmie a new concid"))
            {
                do_makeid(d);
            }
            else if (!strncmp(command, "I wanna connect concid ", sizeof("I wanna connect concid ")-1))
            {
                char *m,*n;
                m = command+sizeof("I wanna connect concid ")-1;
                n = strchr(m,' ');

                if(!n)
                {
                    queue_string(d,"Usage: I wanna connect concid <id> <hostname>\n");
                }
                else
                {
                    do_connectid(d,atoi (command+ sizeof("I wanna connect concid ")-1),n);
                }
            }
            else if (!strncmp(command, "I wanna kill concid ", sizeof("I wanna kill concid ")-1))
            {
                do_killid(d, atoi(command+ sizeof("I wanna kill concid ")-1));
            }
            else
            {
                char *k;
                k = strchr(command, ' ');

                if(!k)
                {
                    queue_string(d, "Huh???\r\n");
                }
                else
                {
                    struct descriptor_data *l;
                    int j;

                    *k = '\0';
                    j=atoi(command);

                    for(l = descriptor_list; l; l = l->next)
                    {
                        if (l->concid == j)
                        {
                            break;
                        }
                    }
                    if(!l)
                    {
                        queue_string(d, "I don't know that concid.\r\n");
                    }
                    else
                    {
                        k++;

                        if (!do_command(l, k))
                        {
                            connect_message(l, leave_msg_file, 1);
                            shutdownsock(l);
                        }
                    }
                }
            }
        }
        else
        {
            if (d->state == CONNECTED)
            {

                if (d->output_prefix)
                {
                    queue_string (d, d->output_prefix);
                    queue_write (d, "\n", 1);
                }

                cplr = d->player;

                if (d->player > 0)
                {
                    strcpy(ccom,command);
                    process_command (d->player, command,NOTHING);
                }
                else
                {
                    send_message(-d->player,command);
                }
 
                if (d->output_suffix)
                {
                    queue_string (d, d->output_suffix);
                    queue_write (d, "\n", 1);
                }
            }
            else
            {
                check_connect (d, command);
            }
        }
    }

    return TRUE;
}

static void check_connect (struct descriptor_data *d, char *msg)
{
    char buf[BUF_SIZE];
    char *p;
    char command[MAX_COMMAND_LEN];
    char user[MAX_COMMAND_LEN];
    char password[MAX_COMMAND_LEN];
    dbref player;
    char *m = NULL;
    int connects, num;

    if (d->state == WAITPASS)
    {
        // msg contains the password. 
        char *foobuf = newglurp(MAX_COMMAND_LEN);
        sprintf(foobuf, "connect %s %s", d->charname, msg);
        free(d->charname);
        queue_string(d, got_password);
        d->state = WAITCONNECT;
        msg = foobuf;
    }

    parse_connect (msg, command, user, password);

    if (!strcmp(command, "WHO"))
    {
        dump_users(0, "", "", d);
    }
    else if (!strncmp (command, "co", 2))
    {
        if ( string_prefix(user, guest_prefix) || string_prefix(user,"guest") )
        {
            strcpy(password, guest_prefix);

            if (check_lockout (d, guest_lockout_file, guest_msg_file))
            {
                player = NOTHING;
            }
            else
            {
                if ( (p = make_guest(d)) == NULL )
                {
                    return;
                }

                strcpy(user, p);
                player = connect_player (user, password);
            }
        }
        else
        {
            if (check_lockout (d, welcome_lockout_file, NULL))
            {
                player = NOTHING;
            }
            else
            {
                player = connect_player (user, password);
            }
        }

        if (player != NOTHING && Typeof(player) == TYPE_PLAYER)
        {
            if (*db[player].pows < restrict_connect_class)
            {
                sprintf(buf, "%s refused connection due to class restriction.", unparse_object(root,player));
                log_io(buf);
                write (d->descriptor, lockout_message, strlen (lockout_message));
                process_output(d);

                d->state        = CONNECTED;
                d->connected_at = time(0);
                d->player       = player;

                shutdownsock(d);
                return;
            }
        }

        if (player == NOTHING && !*password)
        {
            // hmm. bet they want to type it in seperately.
            queue_string(d, get_password);

            d->state    = WAITPASS;
            d->charname = malloc(strlen(user)+1);

            strcpy(d->charname, user);

            return;
        }
        else if (player == NOTHING)
        {
            queue_string (d, connect_fail);
            sprintf(buf, "FAILED CONNECT %s on concid %d", user, d->concid);
            log_io (buf);
        }
        else
        {
            sprintf(buf, "CONNECTED %s(%d) on concid %d", db[player].name, player, d->concid);
            log_io (buf);

            if (!(*atr_get(player,A_LHIDE)))
            {
                sprintf(buf, "[%s] * CONNECT: %s(#%d)", conn_chan, db[player].name, player);
                com_send(conn_chan, buf);
            }

            sprintf(buf, "[%s] * CONNECT: %s(#%d) (concid %d, host %s@%s)", dconn_chan, db[player].name, player, d->concid, d->user, d->addr);
            com_send(dconn_chan, buf);

            if (d->state == WAITPASS)
            {
                queue_string(d, got_password);
            }

            d->state        = CONNECTED;
            d->connected_at = now;
            d->player       = player;

            // give players a message on connection 
            connect_message(d, connect_msg_file, 0);
            announce_connect(player);
            send_message(player, "Connecting..");

            if (!(Guest(player)))
            {
                connects = atoi(atr_get(player, A_CONNECTS));
                connects++;
                sprintf(buf, "%d", connects);
                atr_add(player, A_CONNECTS, buf);

                if (connects == 1)
                {
                    send_message(player, "This is your first time here.  Welcome!");
                }
                else
                {
                    send_message(player, "You've connected %d times... welcome back!", connects);
                }

                send_message(player, "");
            }

            m = atr_get(player, A_LASTSITES);
            num = 0;

            while (*m)
            {
                while (*m && isspace(*m)) m++;
                if (*m) num++;
                while (*m && !isspace(*m)) m++;
            }

            if (num >= 10)
            {
                m = atr_get(player, A_LASTSITES);
                num = 0;

                while (*m && isspace(*m)) m++;
                while (*m && !isspace(*m)) m++;
                sprintf(buf, "%s %s@%s", m, d->user, d->addr);
                atr_add(player, A_LASTSITES, buf);
            }
            else
            {
                m = atr_get(player, A_LASTSITES);

                while (*m && isspace(*m))
                    m++;

                sprintf(buf, "%s %s@%s", m, d->user, d->addr);
                atr_add(player, A_LASTSITES, buf);
            }

            do_look_around(player);
            // XXX might want to add check for haven here.

            if (Guest(player))
            {
                send_message(player, "Welcome to %s; your name is %s", muse_name, db[player].name);
            }
        }
    }
    else if (!strncmp (command, "cr", 2))
    {
        if (!allow_create)
        {
            connect_message(d, register_msg_file, 0);
        }
        else
        {
            player = create_player (user, password,CLASS_VISITOR, player_start);
            if (player == NOTHING)
            {
                queue_string (d, create_fail);
                sprintf(buf, "FAILED CREATE %s on concid %d", user, d->concid);
                log_io (buf);
            }
            else
            {
                sprintf("CREATED %s(%d) on concid %d", db[player].name, player, d->concid);
                log_io (buf);

                d->state = CONNECTED;
                d->connected_at = now;
                d->player = player;

                // give new players a special message
                connect_message(d, create_msg_file, 0);
                // connect_message(d, connect_msg_file);
                announce_connect(player);
                do_look_around(player);
            }
        }
    }
    else
    {
        check_lockout (d, welcome_lockout_file, welcome_msg_file);
    }

    if (d->state == WAITPASS)
    {
        d->state = WAITCONNECT;
        queue_string(d, got_password);
    }
}

static void parse_connect(char *msg, char *command, char *user, char *pass)
{
    char *p;

    while (*msg && isascii(*msg) && isspace (*msg))
    {
        msg++;
    }

    p = command;

    while (*msg && isascii(*msg) && !isspace (*msg))
    {
        *p++ = *msg++;
    }

    *p = '\0';

    while (*msg && isascii(*msg) && isspace (*msg))
    {
        msg++;
    }

    p = user;

    while (*msg && isascii(*msg) && !isspace (*msg))
    {
        *p++ = *msg++;
    }

    *p = '\0';

    while (*msg && isascii(*msg) && isspace (*msg))
    {
        msg++;
    }

    p = pass;

    while (*msg && isascii(*msg) && !isspace (*msg))
    {
        *p++ = *msg++;
    }

    *p = '\0';
}

// this algorithm can be changed later to accomodate an unlimited
// number of guests.  currently, it supports a limited number.
static char *make_guest(struct descriptor_data *d)
{
    char buf[BUF_SIZE];
    int i;
    dbref player;
    static char name[50];
    static char alias[50];

    *name = '\0';

    for ( i = 0; i < number_guests; i++ )
    {
        sprintf(name, "%s%d", guest_prefix, i);

        if (lookup_player(name) == NOTHING)
        {
            break;
        }
    }

    if ( i == number_guests )
    {
        queue_string(d, "All guest ID's are busy; please try again later.\n");

        return NULL;
    }

    player = create_guest(name, guest_prefix);


    if ( player == NOTHING )
    {
        queue_string(d, "Error creating guest ID, please try again later.\n");
        sprintf(buf, "Error creating guest ID.  '%s' already exists.", name);
        log_error(buf);
             
        return NULL;
    }

    sprintf(alias, "%c%d", name[0], i);

    delete_player(player);
    mark_hearing(player);

    if (lookup_player(alias) == NOTHING)
    {
        atr_add(player, A_ALIAS, alias);
    }

    add_player(player);
    check_hearing();

    return db[player].name;
}

static void close_sockets()
{
    struct descriptor_data *d, *dnext;
    FILE *x = NULL;

    if (exit_status == 1)
    {
        unlink("run/logs/socket_table");
        x = fopen("run/logs/socket_table","w");
        fprintf(x,"%d\n",sock);
        fcntl(sock, F_SETFD, 0);
    }

/*  shutdown_flag = 1;        * just in case. so announce_disconnect
                 * does'nt reset the CONNECT
                 * flags. */

    for (d = descriptor_list; d; d = dnext)
    {
        dnext = d->next;

        if (!(d->cstatus&C_REMOTE))
        {
            write (d->descriptor, shutdown_message, strlen (shutdown_message));
            process_output(d);

            if (x && d->player>=0 && d->state==CONNECTED /*&& !Guest(d->player)*/ )
            {
                fprintf(x,"%010d %010ld %010ld %010d\n",d->descriptor, d->connected_at, d->last_time, d->player);
                fcntl(d->descriptor, F_SETFD, 0);
            }
            else
            {
                shutdownsock(d);
            }
        }
    }

    if (x)
    {
        fclose(x);
    }
}

static void open_sockets()
{
    struct descriptor_data *d, *oldd, *nextd;
    FILE *x = NULL;
    char buf[BUF_SIZE];

    if (!(x = fopen("run/logs/socket_table", "r")))
    {
        return;
    }

    unlink("run/logs/socket_table"); // so we don't try to use it again. 
    fgets(buf, BUF_SIZE, x);
    sock = atoi(buf);
    fcntl(sock, F_SETFD, 1);
    close(sock);

    for (sock = 0; sock < 1000; sock++)
    {
        fcntl(sock, F_SETFD, 1);
    }

    while (fgets(buf, BUF_SIZE, x))
    {
        int desc = atoi(buf);
        struct sockaddr_in in;
        extern char *inet_ntoa();
        int namelen = sizeof(in);

        fcntl(desc, F_SETFD, 1);

        getpeername(desc, (struct sockaddr *)&in, (socklen_t *)&namelen);
        {
            char buff[100];
            struct hostent *hent;

            hent = gethostbyaddr ((char *)&(in.sin_addr.s_addr), sizeof (in.sin_addr.s_addr), AF_INET);
               
            if (hent)
            {
                strcpy (buff, hent->h_name);
            }
            else
            {
                extern char *inet_ntoa();
                strcpy(buff, inet_ntoa(in.sin_addr));
            }

            d = initializesock(desc, &in, buff, RELOADCONNECT);
        }

        d->connected_at = atoi(buf+11);
        d->last_time = atoi(buf+22);
        d->player = atoi(buf+33);
    }

    fclose(x);
    oldd = descriptor_list;
    descriptor_list = NULL;

    for (d=oldd; d; d=nextd)
    {
        nextd = d->next;
        d->next = descriptor_list;
        descriptor_list = d;
    }

    oldd = NULL;

    for (d = descriptor_list; d; d = d->next)
    {
        if (oldd == NULL)
        {
            d->prev = &descriptor_list;
        }
        else
        {
            d->prev = &oldd->next;
        }

        oldd = d;
    }
}

void emergency_shutdown()
{
    log_error("Emergency shutdown.");
    shutdown_flag = 1;
    exit_status = 136;
    close_sockets();
}

int boot_off(dbref player)
{
    struct descriptor_data *d;

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->state == CONNECTED && d->player == player)
        {
            process_output (d);
            shutdownsock(d);
            return TRUE;
        }
    }

    return FALSE;
}

static signal_type bailout(int sig)
{
    char message[BUF_SIZE];

    sprintf (message, "BAILOUT: caught signal %d", sig);
    panic(message);
    exit_nicely (136);
#ifdef void_signal_type
    return;
#else
    return 0;
#endif
}

static signal_type do_sig_reboot(int i)
{
    exit_status   = TRUE;
    shutdown_flag = TRUE;
#ifdef void_signal_type
    return;
#else
    return 0;
#endif
}

static signal_type dump_status(int i)
{
    struct descriptor_data *d;
    long now;

    now = time (NULL);
    fprintf (stderr, "STATUS REPORT:\n");

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->state == CONNECTED)
        {
            fprintf (stderr, "PLAYING descriptor %d concid %d player %s(%d)", d->descriptor, d->concid, db[d->player].name, d->player);

            if (d->last_time)
            {
                fprintf (stderr, " idle %ld seconds\n", now - d->last_time);
            }
            else
            {
                fprintf (stderr, " never used\n");
            }
        }
        else
        {
            fprintf (stderr, "CONNECTING descriptor %d concid %d", d->descriptor, d->concid);

            if (d->last_time)
            {
                fprintf (stderr, " idle %ld seconds\n", now - d->last_time);
            }
            else
            {
                fprintf (stderr, " never used\n");
            }
        }
    }
#ifdef void_signal_type
    return;
#else
    return 0;
#endif
}

/////////////////////////////////////////////////////////////////////
// dump_users()
/////////////////////////////////////////////////////////////////////
//     Displays all of the users currently on the MUD
/////////////////////////////////////////////////////////////////////
// Notes: if non-zero, use k instead of w
/////////////////////////////////////////////////////////////////////
// Returns: -
/////////////////////////////////////////////////////////////////////
void dump_users(dbref w, char *arg1, char *arg2, struct descriptor_data *k)
{
    char buf[WHO_BUF_SIZ+1];
    char buf2[BUF_SIZE];
    char *p;
    char *b;
    char *names;
    char fbuf[5];
    char flags[WHO_SIZE+1];
    int see_player;
    int j, ni, bits, num_secs, scr_cols, hc, header;
    int grp, grp_len, num_grps, total_count, hidden_count;
    int done         = 0;
    int pow_who      = 0;
    int hidden       = 0;
    int active_users = 0;
    int color        = 0;
    struct descriptor_data *d;
    dbref who, nl_size, *name_list;

    strcpy(flags, DEF_WHO_FLAGS);

    // setup special data 
    names = "";
    scr_cols = DEF_SCR_COLS;

    if( ! k )
    {
        if( Typeof(w) != TYPE_PLAYER && ! payfor(w, 50) )
        {
            send_message(w, "You don't have enough pennies.");
            return;
        }

        if ( *(p = atr_get(w, A_COLUMNS)) != '\0' )
        {
            scr_cols = atoi(p);
            if ( scr_cols > (WHO_BUF_SIZ + 1) )
            {
                scr_cols = WHO_BUF_SIZ;
            }
        }
        if ( *(p = atr_get(w, A_WHOFLAGS)) != '\0' )
        {
            *flags = '\0';
            strncat(flags, p, WHO_SIZE);
        }
        // in case this value used, we must process
        // names before atr_get is called again 
        if ( *(p = atr_get(w, A_WHONAMES)) != '\0' )
        {
            names = p;
        }
    }

    // override data 
    if ( arg1 != NULL )
    {
        if ( *arg1 != '\0' )
        {
            *flags = '\0';
            strncat(flags, arg1, WHO_SIZE);
        }
    }
    if ( arg2 != NULL )
    {
        if ( *arg2 != '\0' )
        {
            names = arg2;
        }
    }

    // Check for power to distinguish class names 
    pow_who = ( k ) ? 0 : has_pow(w, NOTHING, POW_WHO);

    // process flags 
    bits     = 0;
    grp_len  = 0;
    num_secs = 0;

    for ( p = flags; *p != '\0'; p++ )
    {
        int bit = 1;
        for (int i = 0; i < WHO_SIZE; i++, bit <<= 1 )
        {
            if ( to_lower(*p) == who_flags[i] )
            {
                ++num_secs;
                bits |= bit;
                if ( islower(*p) )
                {
                    who_sizes[i] = who_sizes_small[i];
                    who_fmt[i]   = who_fmt_small[i];
                }
                else
                {
                    who_sizes[i] = who_sizes_large[i];
                    who_fmt[i]   = who_fmt_large[i];
                }

                grp_len += who_sizes[i];
                break;
            }

            if ( i == WHO_SIZE )
            {
                sprintf(buf, "%c: bad who flag.%s", (int) *p, (k) ? "\n" : "");
                if ( k )
                {
                    queue_string(k, buf);
                }
                else
                {
                    send_message(w, buf);
                }
                return;
            }
        }
    }

    // process names (before atr_get called that overwrites static buffer) 
    if ( *names == '\0' )
    {
        // avoid error msg in lookup_players() for missing name list 
        nl_size   = 0;
        name_list = &nl_size;
    }
    else
    {
        name_list = lookup_players((k) ? NOTHING : w, names);

        if (!name_list[0])
        {
            return; // we tried, but no users matched.
        }
    }

    // add in min section spacing 
    grp_len += (num_secs - 1) * MIN_SEC_SPC;

    // calc # of groups 
    num_grps = ((scr_cols - grp_len) / (grp_len + MIN_GRP_SPC)) + 1;
    if ( num_grps < 1 )
    {
        num_grps = 1;
    }

    // calc header size based on see-able users 
    for ( hc = 0, d = descriptor_list; d; d = d->next )
    {
        if ( d->state != CONNECTED || d->player <= 0 )
        {
            continue;
        }
        if (*atr_get(d->player, A_LHIDE))
        {
            if (k)
            {
                continue;
            }
            if (!controls(w, d->player, POW_WHO) && !could_doit(w, d->player, A_LHIDE))
            {
                continue;
            }
        }
        if ( name_list[0] > 0 )
        {
            for (int i = 1; i <= name_list[0]; i++ )
            {
                if ( d->player == name_list[i] )
                {
                    break;
                }
                // Belisarius
                // this if statement not originally part of
                // for loop, may need to be outside of lookup_player
                // was ambiguous due to no use of curlybraces
                if ( i > name_list[0] )
                {
                    continue;
                }
            }
        }
        if ( ++hc >= num_grps )
        {
            break;
        }
    }

    // process names or, (if no names requested) loop once for all names 
    b      = buf;
    grp    = 1;
    header = 1;
    ni     = 1;
    who    = NOTHING;
    done   = 0;
    total_count  = 0;
    hidden_count = 0;

    while ( !done )
    {
        // if a player is on the names list, process them 
        if ( ni <= name_list[0] )
        {
            who = name_list[ni++];
        }

        // if no names left on list, this is final run through loop 
        if ( ni > name_list[0] )
        {
            done = 1;
        }

        // Build output lines and print them 
        d = descriptor_list;

        while ( d )
        {
            if ( !header )
            {
                // skip records that aren't in use 
                // if name list, skip those not on list 
                if ( d->state != CONNECTED || d->player <= 0 || ( who != NOTHING && who != d->player) )
                {
                    d = d->next;
                    continue;
                }

                // handle hidden players, keep track of player counts 
                hidden     = 0;
                see_player = 1;

                if (k ? *atr_get(d->player, A_LHIDE) : !could_doit(w, d->player, A_LHIDE))
                {
                    hidden = 1;
                    see_player = !k && controls(w, d->player, POW_WHO);
                }

                if ((now - d->last_time) < 300)
                {
                    active_users++;
                }

                // do counts 
                if ( see_player || name_list[0] == 0 )
                {
                    ++total_count;
                    if ( hidden )
                    {
                        ++hidden_count;
                    }
                }

                if ( !see_player )
                {
                    d = d->next;
                    continue;
                }
            }

            if ( ! k)
            {
                if (header)
                {
                    sprintf(b, "|%s|", atr_get(0, A_COLOR));
                    b += strlen(b);
                }
                else
                {
                    if(color)
                    {
                        sprintf(b, "|%s|", atr_get(db[0].zone, A_COLOR));
                        b += strlen(b);
                        color = 0;
                    }
                    else
                    {
                        sprintf(b, "|BRIGHT,%s|", atr_get(db[0].zone, A_COLOR));
                        b += strlen(b);
                        color = 1;
                    }
                }
            }

            // build output line 
            int bit = 1;

            for (int i = 0; i < WHO_SIZE; i++, bit <<= 1 )
            {
                switch ( bits & bit )
                {
                    case W_NAME:
                        p = header ? "Name" : db[d->player].name;
                        break;
                    case W_ALIAS:
                        if ( header )
                            p = "Alias";
                        else if (Typeof(d->player) != TYPE_PLAYER)
                        {
                            sprintf(buf2, "#%d", d->player);
                            p = buf2;
                        }
                        else if ( *(p = atr_get(d->player, A_ALIAS)) == '\0' )
                            p = DEF_WHO_ALIAS;
                        break;
                    case W_FLAGS:
                        if ( header )
                            p = "Flg";
                        else
                        {
                            fbuf[0] = (hidden)                                ? 'h' : ' ';
                            fbuf[1] = (k?*atr_get(d->player,A_LPAGE):
                                       !could_doit(w,d->player,A_LPAGE))      ? 'H' : ' ';
                            fbuf[2] = (db[d->player].flags & PLAYER_NO_WALLS) ? 'N' : ' ';
                            fbuf[3] = (db[d->player].flags & PLAYER_NEWBIE)   ? 'n' : ' ';
                            fbuf[4] = '\0';
                            p = fbuf;
                        }
                        break;
                    case W_DOING:
                        if (header)
                        {
                            p = "Doing";
                        }
                        else
                        {
                            p = atr_get(d->player, A_DOING);
                        }
                        break;
                    case W_ONFOR:
                        p = header ? "On For" : time_format_1(now - d->connected_at);
                        break;
                    case W_IDLE:
                        p = header ?   "Idle" : time_format_2(now - d->last_time);
                        break;
                    case W_CONCID:
                        if ( header )
                        {
                            p = "Concid";
                        }
                        else
                        {
                            if ( has_pow(w, d->player, POW_HOST) )
                            {
                                sprintf(buf2, "%d", d->concid);
                                p = buf2;
                            }
                            else
                            {
                                p = "?";
                            }
                        }
                        break;
                    case W_HOST:
                        p = "?";
                        if ( header )
                        {
                            p = "Hostname";
                        }
                        else if ( !k )
                        {
                            if ( has_pow(w, d->player, POW_HOST) )
                            {
                                sprintf(buf2, "%s@%s", d->user, d->addr);
                                p = buf2;

                                if ( who_sizes[i] > who_sizes_small[i] )
                                {
                                    who_bh_buf[0] = '[';

                                    // (Belisarius) 
                                    // p has just been assigned this exact same string just
                                    // prior to reaching this if block
                                    // for ( j = 1, p = tprintf("%s@%s", d->user, d->addr); j <= 30 && *p != '\0'; j++, p++ )
                                    for ( j = 1; j <= 30 && *p != '\0'; j++, p++ )
                                    {
                                        who_bh_buf[j] = *p;
                                    }
                                    who_bh_buf[j++] = ']';
                                    who_bh_buf[j] = '\0';
                                    p = who_bh_buf;
                                }
                            }
                            else
                            {
                                p = "?";
                            }
                        }
                        break;
                    case W_PORT:
                        if (header)
                        {
                            p = "Port";
                        }
                        else
                        {
                            if ( has_pow(w, d->player, POW_HOST) )
                            {
                                sprintf(buf2, "%d", ntohs(d->address.sin_port));
                                p = buf2;
                            }
                        }
                        break;
                    case W_CLASS:
                        if ( header )
                        {
                            p = "Class";
                        }
                        else
                        {
                            p = wholev(d->player, (who_sizes[i] > who_sizes_small[i]), pow_who);
                        }
                        break;
                    default:
                        continue;
                }

                sprintf(b, who_fmt[i], p);
                b += who_sizes[i] + 1;
           }
           b--;

           if ( header )
           {
               --hc;
           }

           if ( (!header && ++grp <= num_grps) || (header && hc) )
           {
               // make space between groups 
               strcpy(b, "    ");
               b += 4;
           }
           else
           {
               // trim excess spaces 
               while ( *--b == ' ' )
               ;

               // print output line 
               if ( k )
               {
                   *++b = '\n';
                   *++b = '\0';
                   queue_string(k, buf);
               }
               else
               {
                   *++b = '\0';
                   send_message(w, buf);
               }

               // reset for new line 
               grp = 1;
               b = buf;
           }

           if ( header )
           {
               if ( grp == 1 && hc <= 0 )
               {
                   hc = header = 0;
               }
           }
           else
           {
               d = d->next;
           }
        }
    }

    // print last line (if incomplete) 
    if ( grp > 1 )
    {
        // trim extra spaces 
        while ( *--b == ' ' )
        ;

        if ( k )
        {
            *++b = '\n';
            *++b = '\0';
            queue_string(k, buf);
        }
        else
        {
            *++b = '\0';
            send_message(w, buf);
        }
    }

    b = buf;

    if ( hidden_count > 0 )
    {
        if ( ! k)
        {
            sprintf(b, "|%s|Hidden Users%s: |BRIGHT,WHITE|%d   ", atr_get(0, A_COLOR), ( name_list[0] > 0 ) ? " Found" : "", hidden_count);
        }
        else
        {
            sprintf(b, "Hidden Users%s: %d   ", ( name_list[0] > 0 ) ? " Found" : "", hidden_count);
        }

        b = b + strlen(b);
    }

    if ( ! k )
    {
        sprintf(b, "|%s|Total Users%s: |BRIGHT,WHITE|%d.", atr_get(0, A_COLOR), ( name_list[0] > 0 ) ? " Found" : "", total_count);
        sprintf((b + strlen(b)), "   |%s|Active users: |BRIGHT,WHITE|%d.", atr_get(0, A_COLOR), active_users);
    }
    else
    {
        sprintf(b, "Total Users%s: %d.", ( name_list[0] > 0 ) ? " Found" : "", total_count);
        sprintf((b + strlen(b)),"   Active users: %d.", active_users);
    }

    if ( k )
    {
        queue_string(k, "---\n");
        queue_string(k, buf);
        queue_string(k, "\n");
    }
    else
    {
        send_message(w, "---");
        send_message(w, buf);
    }
}

char *time_format_1(time_t dt)
{
    struct tm *delta;
    static char buf[64];

    if (dt < 0)
    {
        dt = 0;
    }

    delta = gmtime(&dt);

    if (delta->tm_yday > 0)
    {
        sprintf(buf, "%dd %02d:%02d", delta->tm_yday, delta->tm_hour, delta->tm_min);
    }
    else
    {
        sprintf(buf, "%02d:%02d", delta->tm_hour, delta->tm_min);
    }

    return buf;
}

char *time_format_2(time_t dt)
{
    register struct tm *delta;
    static char buf[64];

    if (dt < 0)
    {
        dt = 0;
    }

    delta = gmtime(&dt);

    if (delta->tm_yday > 0)
    {
        sprintf(buf, "%dd", delta->tm_yday);
    }
    else if (delta->tm_hour > 0)
    {
        sprintf(buf, "%dh", delta->tm_hour);
    }
    else if (delta->tm_min > 0)
    {
        sprintf(buf, "%dm", delta->tm_min);
    }
    else
    {
        sprintf(buf, "%ds", delta->tm_sec);
    }

    return buf;
}

void announce_connect(dbref player)
{
    dbref loc;
    char buf[MAX_CMD_BUF];
    extern dbref speaker;
    char *s, t[30];       // we put this here instead of in connect_player cuz we gotta 
    time_t tt;            // notify the player and the descriptor isn't set up yet then 
    int connect_again = db[player].flags & CONNECT;

    if ((loc = getloc(player)) == NOTHING) return;

    if (connect_again)
    {
        sprintf(buf, "%s has reconnected.", db[player].name);
    }
    else
    {
        sprintf(buf, "%s has connected.", db[player].name);
    }

    // added to allow player's inventory to hear a player connect 
    speaker = player;

    notify_in(player, player, buf);

    if (!IS(loc, TYPE_ROOM, ROOM_AUDITORIUM))
    {
        notify_in(loc, player, buf);
    }

    db[player].flags |= CONNECT;

    if (Typeof(player) == TYPE_PLAYER)
    {
        db[player].flags&=~HAVEN;
    }

    if (!Guest(player))
    {
        tt = now;
        strcpy(t,ctime(&tt));
        t[strlen(t)-1] = 0;
        tt = atol(atr_get(player, A_LASTCONN));

        if ( tt == 0L )
        {
            s = "no previous login";
        }
        else
        {
            s = ctime(&tt);
            s[strlen(s)-1]=0;

            if ((strncmp(t, s, 10) != 0) && power(player, POW_MEMBER) && db[player].owner == player)
            {
                giveto(player, allowance);
                send_message(player, "You collect %d credits.", allowance);
            }
        }

        send_message(player, "Last login: %s", s);
        tt = now;
        sprintf(buf, "%ld", tt);
        atr_add(player, A_LASTCONN, buf);
        check_mail(player);

    }
    if (!connect_again)
    {
        dbref thing;
        dbref zone;
        dbref who = player;
        int depth;
        int find=0;

        did_it(who, who, NULL, NULL, A_OCONN, NULL, A_ACONN);
        did_it(who, db[who].location, NULL, NULL, NULL, NULL, A_ACONN);

        zone = db[0].zone;

        if (Typeof(db[who].location) == TYPE_ROOM)
        {
            zone=db[db[who].location].zone;
        }
        else
        {
            thing = db[who].location;

            for (depth = 10; depth && (find != 1); depth--, thing = db[thing].location)
            {
                if (Typeof(thing) == TYPE_ROOM)
                {
                    zone=db[thing].zone;
                    find=1;
                }
            }
        }

        if ((db[0].zone != zone) && (Typeof(db[0].zone) != TYPE_PLAYER))
        {
            did_it(who, db[0].zone, NULL, NULL, NULL, NULL, A_ACONN);
        }

        if (Typeof(zone) != TYPE_PLAYER)
        {
            did_it(who, zone, NULL, NULL, NULL, NULL, A_ACONN);
        }

        if ((thing = db[who].contents) != NOTHING)
        {
            DOLIST(thing, thing)
            {
                if (Typeof(thing) != TYPE_PLAYER)
                {
                    did_it(who, thing, NULL, NULL, NULL, NULL, A_ACONN);
                }
            }
        }

        if ((thing = db[db[who].location].contents) != NOTHING)
        {
            DOLIST(thing,thing)
            {
                if (Typeof(thing) != TYPE_PLAYER)
                {
                    did_it(who, thing, NULL, NULL, NULL, NULL, A_ACONN);
                }
            }
        }
    }
}

void announce_disconnect(dbref player)
{
    dbref loc;
    int num;
    char buf[MAX_CMD_BUF];
    char buf2[BUF_SIZE];
    struct descriptor_data *d;
    extern dbref speaker;
    int partial_disconnect;

    if (player < 0)
    {
        return;
    }

    for (num = 0, d = descriptor_list; d; d = d->next)
    {
        if (d->state==CONNECTED && (d->player>0) && (d->player==player))
        {
            num++;
        }
    }

    if (num < 2 && !shutdown_flag)
    {
        db[player].flags&=~CONNECT;
        atr_add(player,A_IT,"");
        partial_disconnect = 0;
    }
    else
    {
        partial_disconnect = 1;
    }

    sprintf(buf2, "%ld", now);
    atr_add(player, A_LASTDISC, buf2);

    if ((loc = getloc(player)) != NOTHING)
    {
        if (partial_disconnect)
        {
            sprintf(buf, "%s has partially disconnected.", db[player].name);
        }
        else
        {
            sprintf(buf, "%s has disconnected.", db[player].name);
        }

        speaker=player;

        // added to allow player's inventory to hear a player connect 
        notify_in(player, player, buf);

        if(!IS(loc,TYPE_ROOM,ROOM_AUDITORIUM))
        {
            notify_in(loc, player, buf);
        }

        if (!partial_disconnect)
        {
            dbref zone;
            dbref thing;
            dbref who = player;
            int depth;
            int find = 0;

            did_it(who, who, NULL, NULL, A_ODISC, NULL, A_ADISC);
            did_it(who, db[who].location, NULL, NULL, NULL, NULL, A_ADISC);

            zone = db[0].zone;

            if (Typeof(db[who].location) == TYPE_ROOM)
            {
                zone = db[db[who].location].zone;
            }
            else
            {
                thing = db[who].location;

                for (depth = 10; depth && (find != 1); depth--, thing = db[thing].location)
                {
                    if (Typeof(thing) == TYPE_ROOM)
                    {
                        zone = db[thing].zone;
                        find = 1;
                    }
                }
            }

            if ((db[0].zone != zone) && (Typeof(db[0].zone) != TYPE_PLAYER))
            {
                did_it(who, db[0].zone, NULL, NULL, NULL, NULL, A_ADISC);
            }
   

            if (Typeof(zone) != TYPE_PLAYER)
            {
                did_it(who, zone, NULL, NULL, NULL, NULL, A_ADISC);

                if ((thing = db[who].contents) != NOTHING)
                {
                    DOLIST(thing,thing)
                    {
                        if (Typeof(thing) != TYPE_PLAYER)
                        {
                            did_it(who, thing, NULL, NULL, NULL, NULL, A_ADISC);
                        }
                    }
                }

                if ((thing = db[db[who].location].contents) != NOTHING)
                {
                    DOLIST(thing,thing)
                    {
                        if (Typeof(thing) != TYPE_PLAYER)
                        {
                            did_it(who, thing, NULL, NULL, NULL, NULL, A_ADISC);
                        }
                    }
                }
            }
        }
    }
}

struct ctrace_int
{
  struct descriptor_data *des;
  struct ctrace_int **children;
};

static struct ctrace_int *internal_ctrace(struct descriptor_data *parent)
{
    struct descriptor_data *k;
    struct ctrace_int *op;
    int nchild;

    op = newglurp(sizeof(struct ctrace_int));
    op->des = parent;

    if (parent && !(parent->cstatus & C_CCONTROL))
    {
        op->children = newglurp(sizeof(struct ctrace_int *));
        op->children[0] = 0;
    }
    else
    {
        for (nchild = 0, k = descriptor_list; k; k = k->next)
        {
            if (k->parent == parent)
            {
                nchild++;
            }
        }

        op->children = newglurp(sizeof(struct ctrace_int *)*(nchild+1));

        for (nchild = 0, k = descriptor_list; k; k = k->next)
        {
            if (k->parent == parent)
            {
                op->children[nchild] = internal_ctrace(k);
                nchild++;
            }

            op->children[nchild] = 0;
        }
    }

    return op;
}

static void ctrace_notify_internal(dbref player, struct ctrace_int *d, int dep)
{
    char buf[BUF_SIZE*2];
    char buf2[BUF_SIZE];
    int j,k;

    for (j = 0; j < dep; j++)
    {
        buf[j]='.';
    }

    if (d->des && dep)
    {
        sprintf(buf2, "\"%s\"", unparse_object(player, d->des->player));

        sprintf(buf+j,"%s descriptor: %d, concid: %d, host: %s@%s",
        (d->des->state==CONNECTED)
        ?(buf2)
        :((d->des->cstatus&C_CCONTROL)
          ?"<Concentrator Control>"
          :"<Unconnected>"),
        d->des->descriptor, d->des->concid,
        d->des->user, d->des->addr);

        send_message(player, buf);
    }

    for (k = 0; d->children[k]; k++)
    {
        ctrace_notify_internal(player, d->children[k], dep+1);
    }
}

void do_ctrace(dbref player)
{
    struct ctrace_int *dscs;

    if (!power(player,POW_HOST))
    {
        send_message(player, "Sorry.");
        return;
    }

    dscs = internal_ctrace(0);
    ctrace_notify_internal(player, dscs, 0);
}

static struct descriptor_data *open_outbound(dbref player, char *host, int port)
{
    struct descriptor_data *d;
    int sock;
    struct sockaddr_in addr;
    struct hostent *hent;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (!(hent=gethostbyname(host)))
    {
        addr.sin_addr.s_addr = inet_addr(host);
    }
    else
    {
        addr.sin_addr.s_addr = *(long *)hent->h_addr_list[0];
    }

    if (connect (sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(sock);
        return 0;
    }

    d = initializesock (sock, &addr, host, CONNECTED);
    d->player = player;
    d->last_time = d->connected_at = now;
    db[d->player].flags |= CONNECT;

    return d;
}

void do_outgoing(dbref player, char *arg1, char *arg2)
{
    dbref thing1;
    char host[BUF_SIZE];
    char buf[BUF_SIZE];
    int port;
    struct descriptor_data *d;

    if (!power(player, POW_OUTGOING))
    {
        send_message(player, perm_denied());
        return;
    }

    if ((thing1 = match_controlled(player, arg1, POW_BOOT)) == NOTHING)
    {
        return;
    }

    if (!*atr_get(thing1, A_INCOMING))
    {
        send_message(player,"that's not wise.");
        return;
    }

    if (!*arg2 || !strchr(arg2,' '))
    {
        send_message(player,"go away.");
        return;
    }

    strcpy(host, arg2);
    *strchr(host, ' ') = '\0';
    port = atoi(strchr(arg2,' ')+1);

    if (port <= 0)
    {
        send_message(player,"bad port.");
        return;
    }

    if ((d = open_outbound(thing1, host, port)))
    {
        did_it(player, thing1, NULL, NULL, NULL, NULL, A_ACONN);
        sprintf(buf, "%s opened outbound connection to %s, concid %d, attached to %s",unparse_object_a(root,player), arg2, d->concid, unparse_object_a(root, thing1));
        log_io(buf);
    }
    else
    {
        send_message(player, "problems opening connection. errno %d.", errno);
    }
}

static int check_lockout (struct descriptor_data *d, char *file, char *default_msg)
{
    FILE *f;
    char *lock_host, *lock_enable, *msg_file, *ptr;
    char buf[BUF_SIZE];
    struct hostent *hent, *ohent;
    long ohent_addr;
    close(reserved);

    ohent = gethostbyname(d->addr);
    if (ohent)
        ohent_addr = *(long *)ohent->h_addr;
    else
        ohent_addr = 0;

    f = fopen(file,"r");

    if (!f)
    {
         queue_string(d,"error opening lockout file.\n");
         return 1;
    }

    while (fgets(buf, BUF_SIZE, f))
    {
        if (*buf) buf[strlen(buf)-1] = '\0';
          ptr = buf;
        if (!(lock_host = parse_up(&ptr, ' ')))
            continue;
        if (!(lock_enable = parse_up(&ptr, ' ')))
            continue;
        if (!(msg_file = parse_up(&ptr, ' ')))
            continue;
        if (parse_up(&ptr, ' '))
            continue;

        hent = gethostbyname(lock_host);

        if (!hent) continue;

        if (*(long *)hent->h_addr == ohent_addr)
        {
             // bingo. 
             fclose(f);
             connect_message(d, msg_file, 0);

             return *lock_enable == 'l' || *lock_enable == 'L';
        }
    }

    fclose(f);

    if (default_msg != NULL)
    {
         connect_message(d, default_msg, 0);
    }

    return 0;
}

char *wholev(dbref who, int verbose, int iswiz)
{
    static char buf[50];

    if ( Typeof(who) != TYPE_PLAYER )
        strcpy(buf, "*****");
    else if ( iswiz && verbose )
        strcpy(buf, class_to_name(*db[who].pows));
    else if ( iswiz )
        strcpy(buf, short_class_to_name(*db[who].pows));
    else if ( verbose )
        strcpy(buf, public_class_to_name(*db[who].pows));
    else
        strcpy(buf, short_public_class_to_name(*db[who].pows));

    return buf;
}

void do_nologins(dbref player, char *arg1)
{
    char buf[BUF_SIZE];

    sprintf(buf, "%s tries: @nologins %s", unparse_object(root, player), arg1);
    log_sensitive(buf);
               
    if (!power(player,POW_SECURITY))
    {
        send_message(player, "Permission denied.");
        return;
    }

    if (!string_compare(arg1,"on"))
    {
        if (nologins)
        {
            send_message(player,"@nologins has already been enabled.");
            return;
        }
        else
        {
            nologins = TRUE;
            send_message(player,"@nologins has been enabled. No one may log in now.");
            sprintf(buf, "%s executes: @nologins %s", unparse_object(root, player), arg1);
            log_sensitive(buf);
        }
    }
    else if (!string_compare(arg1,"off"))
    {
        if (!nologins)
        {
            send_message(player,"@nologins has already been disabled.");
            return;
        }
        else
        {
            nologins = FALSE;
            send_message(player,"@nologins has been disabled. Logins will now be processed.");
            sprintf(buf, "%s executes: @nologins %s", unparse_object(root, player), arg1);
            log_sensitive(buf);
        }
    }
    else
    {
        switch(nologins)
        {
            case 0:
                send_message(player, "@nologins is disabled.");
                break;
            case 1:
                send_message(player, "@nologins is enabled.");
                break;
            default:
                send_message(player, "Value for @nologins is: %d", nologins);
                break;
        }
    }
}

void do_usercap(dbref player, char *arg1)
{
    char buf[BUF_SIZE];
    int limit = atoi(arg1);

    sprintf(buf, "%s tries: @usercap %s",unparse_object(root,player), arg1);
    log_sensitive(buf);
               
    if (!power(player,POW_SECURITY))
    {
        send_message(player, "Permission denied.");
        return;
    }

    if (limit > 0)
    {
        send_message(player, "Usercap reset to %d players.", limit);
        user_limit = limit;
        sprintf(buf, "%s executes: @usercap %d", unparse_object(root,player), limit);
        log_sensitive(buf);
    }
    else
    {
        send_message(player, "Current usercap is %d users.",user_limit);
    }
}

void do_lockout(dbref player, char *arg1)
{
    char buf[BUF_SIZE];
    int new;
    char temp[100];

    if (!power(player, POW_SECURITY))
    {
        send_message(player, "Permission denied.");
        return;
    }

    if (*arg1)
    {
        if (strcmp(arg1,"none") == 0)
        {
            send_message(player,"Connection restrictions have been lifted.");
            restrict_connect_class = 0;
            sprintf(buf, "%s removes class based connection restrictions.", unparse_object(root, player));
            log_sensitive(buf);
        }
        else
        {
            new = name_to_class(arg1);

            if (new == 0)
            {
                send_message(player, "Unknown class!");
            }
            else
            {
                restrict_connect_class = new;

                send_message(player, "Users below %s are now being rejected!", class_to_name(new));
                sprintf(buf, "%s restricts user access to %s and above.", unparse_object(root, player), class_to_name(new));
                log_sensitive(buf);
            }
        }
    }
    else
    {
        if (restrict_connect_class == 0)
        {
            strcpy(temp, "No class-lockout is in effect.");
        }
        else
        {
            sprintf(temp, "Currently locking out all users below %s.", class_to_name(restrict_connect_class));
        }

        send_message(player, temp);

        if (restrict_connect_class != 0)
        {
            send_message(player,"To remove restrictions, type: @lockout none");
        }
    }
}
