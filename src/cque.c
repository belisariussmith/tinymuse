// cque.c 
// $Id: cque.c,v 1.9 1993/08/16 01:57:18 nils Exp $ 

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "interface.h"
#include "match.h"
#include "externs.h"

// External declarations
extern char ccom[];
extern dbref cplr;

// Local globals
typedef struct bque BQUE;

static void big_que P((dbref, char *, dbref));

struct bque
{
    BQUE *next;
    dbref player;  // player who will do command 
    dbref cause;   // player causing command (for %n) 
    int left;      // seconds left until execution 
    char *env[10]; // environment, from wild match 
};

//static NALLOC *big=NULL;
static BQUE *qfirst  = NULL, *qlast  = NULL, *qwait = NULL;
static BQUE *qlfirst = NULL, *qllast = NULL;

void parse_que(dbref player, char *command, dbref cause)
{
    // cause is needed to determine priority 
    char buff[BUF_SIZE*2],*s,*r;

    s = buff;
    strcpy(buff,command);

    while ((r=(char *)parse_up(&s,';')))
    {
        big_que(player,r,cause);
    }
}

static int add_to(dbref player, int am)
{
    int num;
    char buff[20];

    player = db[player].owner;
    num    = atoi(atr_get(player, A_QUEUE));
    num   += am;

    if (num)
    {
        sprintf(buff, "%d", num);
    }
    else
    {
        *buff = 0;
    }

    atr_add(player,A_QUEUE,buff);

    return(num);
}

static void big_que(dbref player, char *command, dbref cause)
{
    int a;
    BQUE *tmp;
    //  if (!big)
    //  big=nza_open(1);

    if (db[player].flags & HAVEN)
    {
        return;
    }

    // make sure player can afford to do it 
    if (!payfor(player,queue_cost+(((rand() & queue_loss)==0) ? 1 : 0)))
    {
        send_message(db[player].owner, "Not enough money to queue command.");
        return;
    }

    if (add_to(player,1)>max_queue)
    {
        send_message(db[player].owner, "Run away object (%s), commands halted", unparse_object(db[player].owner, player));
        do_halt(db[player].owner, "");
        // haven also means no command execution allowed 
        db[player].flags |= HAVEN;

        return;
    }

    tmp = (BQUE *) malloc(sizeof(BQUE) + strlen(command)+1);

    strcpy(Astr(tmp),command);

    tmp->player = player;
    tmp->next   = NULL;
    tmp->cause  = cause;

    for (a = 0; a < 10; a++)
    {
        if (!wptr[a])
        {
            tmp->env[a] = NULL;
        }
        else
        {
            strcpy(tmp->env[a] = (char *)malloc(strlen(wptr[a])+1), wptr[a]);
        }
    }

    if (Typeof(cause) == TYPE_PLAYER)
    {
        if (qlast)
        {
            qlast->next = tmp;
            qlast = tmp;
        }
        else
        {
            qlast = qfirst = tmp;
        }
    }
    else
    {
        if (qllast)
        {
              qllast->next = tmp;
              qllast = tmp;
        }
        else
        {
              qllast = qlfirst = tmp;
        }
    }
}

void wait_que(dbref player, int wait, char *command, dbref cause)
{
    BQUE *tmp;
    int a;

    // make sure player can afford to do it 
    if (!payfor(player, queue_cost + (((rand() & queue_loss) == 0) ? 1 : 0)))
    {
        send_message(player, "Not enough money to queue command.");
        return;
    }

    tmp = (BQUE *) malloc(sizeof(BQUE) + strlen(command) + 1);

    strcpy(Astr(tmp),command);

    tmp->player  = player;
    tmp->next    = qwait;
    tmp->left    = wait;
    tmp->cause   = cause;

    for (a = 0; a < 10; a++)
    {
        if (!wptr[a])
        {
            tmp->env[a] = NULL;
        }
        else
        {
            strcpy(tmp->env[a] = (char *)malloc(strlen(wptr[a])+1), wptr[a]);
        }
    }

    qwait=tmp;
}

///////////////////////////////////////////////////////////////////////////////
// do_second()
///////////////////////////////////////////////////////////////////////////////
// Call every second to check for wait queue commands
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
void do_second()
{
    BQUE *trail = NULL;
    BQUE *point;
    BQUE *next;

    // move contents of low priority queue onto end of normal one 
    // this helps to keep objects from getting out of control since 
    // its affects on other objects happen only after one seconds 
    // this should allow @halt to be type before getting blown away 
    // by scrolling text 
    if (qlfirst)
    {
        if (qlast)
        {
            qlast->next = qlfirst;
        }
        else
        {
            qfirst = qlfirst;
        }

        qlast   = qllast;
        qlfirst = NULL;
        qllast  = qlfirst;
    }

    // Note: this would be 0 except the command is being put in the low
    // priority queue to be done in one second anyways
    for (point = qwait; point; point = next)
    {
        if (point->left-- <= 1)
        {
            int a;
            giveto(point->player, queue_cost);

            for (a = 0; a < 10; a++)
            {
                wptr[a] = point->env[a];
            }

            parse_que(point->player, Astr(point), point->cause);

            if (trail)
            {
                trail->next = next = point->next;
            }
            else
            {
                qwait = next = point->next;
            }

            for (a = 0; a < 10; a++)
            {
                if (point->env[a])
                {
                    free(point->env[a]);
                }
            }

            free(point);
        }
        else
        {
            next = (trail = point)->next;
        }
    }
}

int test_top()
{
    return (qfirst ? TRUE : FALSE);
}

// execute one command off the top of the queue 
int do_top()
{
    int a;
    BQUE *tmp;

    dbref player;

    if (!qfirst)
    {
        return(FALSE);
    }

    if (qfirst->player!=NOTHING && !(db[qfirst->player].flags & GOING))
    {
        giveto(qfirst->player, queue_cost);

        cplr = qfirst->player;

        strcpy(ccom, Astr(qfirst));
        add_to(player = qfirst->player, -1);

        qfirst->player = NOTHING;

        if (!(db[player].flags & HAVEN))
        {
            char buff[BUF_SIZE+6];
            extern FILE *command_log;

            int a;

            for (a = 0; a < 10; a++)
            {
                wptr[a] = qfirst->env[a];
            }

            if (command_log)
            {
                fprintf(command_log, ">%s<", Astr(qfirst));
            }

            func_zerolev();
            pronoun_substitute(buff, qfirst->cause, Astr(qfirst), player);

            inc_qcmdc();        // increment command stats 

            process_command(player, buff+strlen(db[qfirst->cause].name), qfirst->cause);
        }
    }

    tmp = qfirst->next;

    for (a = 0; a < 10; a++)
    {
        if (qfirst->env[a])
        {
            free(qfirst->env[a]);
        }
    }

    free(qfirst);

    if (!(qfirst = tmp))
    {
        qlast = NULL;
    }

  return(TRUE);
}

// tell player what commands they have pending in the queue 
void do_queue(dbref player)
{
    BQUE *tmp;
    int can_see = power(player, POW_QUEUE);

    send_message(player,"Immediate commands:");

    for (tmp = qfirst; tmp; tmp = tmp->next)
    {
        if ((db[tmp->player].owner == db[player].owner) || can_see)
        {
            send_message(player, "%s:%s", unparse_object(player, tmp->player), Astr(tmp));
        }
    }

    for (tmp = qlfirst; tmp; tmp = tmp->next)
    {
        if ((db[tmp->player].owner == db[player].owner) || can_see)
        {
            send_message(player, "%s:%s", unparse_object(player, tmp->player), Astr(tmp));
        }
    }

    send_message(player,"@waited commands:");

    for (tmp = qwait; tmp; tmp = tmp->next)
    {
        if ((db[tmp->player].owner == db[player].owner) || can_see)
        {
            send_message(player, "%s:%d:%s", unparse_object(player, tmp->player), tmp->left, Astr(tmp));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// do_haltall()
///////////////////////////////////////////////////////////////////////////////
// Halt everything a player does, period
///////////////////////////////////////////////////////////////////////////////
// Returns; -
///////////////////////////////////////////////////////////////////////////////
void do_haltall(dbref player)
{
    BQUE *i;
    BQUE *next;

    // Check for database
    if (!db)
    {
        // No database, nothing to do
        return;
    }

    if (!power(player, POW_SECURITY))
    {
        send_message(player, "silly %s. you can't halt everything.", db[player].name);
        return;
    }

    if (!qlast)
    {
        qfirst = qlast = malloc(sizeof(BQUE));
    }

    if (!qllast)
    {
        qlfirst = qllast = malloc(sizeof(BQUE));
    }

    qlast->next  = qlfirst;
    qllast->next = qwait;

    for (i = qfirst; i; i = next )
    {
        next = i->next;
        free(i);
    }

    qlfirst = qllast = qfirst = qlast = qwait = NULL;
    send_message(player, "everything halted. every single thing. sigh. sniff. no more stuff to process. sniff sniff.");
}

// remove all queued commands from a certain player 
void do_halt(dbref player, char *ncom)
{
    //BQUE *tmp   = NULL;
    //BQUE *point;
    BQUE *trail = NULL;
    BQUE *next;
    int num = 0;

    if (!(db[player].flags & QUIET))
    {
        if (player == db[player].owner)
        {
            send_message(db[player].owner, "Everything halted.");
        }
        else
        {
            if(!(db[db[player].owner].flags & QUIET))
            {
                send_message(db[player].owner, "%s halted.", unparse_object(db[player].owner, player));
            }
        }
    }

    for (BQUE *tmp = qfirst; tmp; tmp = tmp->next)
    {
        if ((tmp->player == player) || (db[tmp->player].owner == player))
        {
            num--;
            giveto(player, queue_cost);
            tmp->player = NOTHING;
        }
    }

    for (BQUE *tmp = qlfirst; tmp; tmp = tmp->next)
    {
        if ((tmp->player == player) || (db[tmp->player].owner == player))
        {
            num--;
            giveto(player, queue_cost);
            tmp->player = NOTHING;
        }
    }

    // remove wait q stuff
    for (BQUE *point = qwait; point; point = next)
    {
        if ((point->player == player) || (db[point->player].owner == player))
        {
            int a;

            giveto(point->player, queue_cost);

            if (trail)
            {
                trail->next = next = point->next;
            }
            else
            {
                qwait = next = point->next;
            }

            for (a = 0; a < 10; a++)
            {
                if (point->env[a])
                {
                    free(point->env[a]);
                }
            }

            free(point);
        }
        else
        {
            next = (trail = point)->next;
        }
    }

    if (db[player].owner == player)
    {
        atr_add(player, A_QUEUE, "");
    }
    else
    {
        add_to(player, num);
    }

    if (*ncom)
    {
        parse_que(player, ncom, player);
    }
}
