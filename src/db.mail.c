#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tinymuse.h"
#include "externs.h"
#include "db.h"

// Definitions (should be placed into db.h) -- Belisarius
#define NOMAIL ((mdbref)-1)

#define MF_DELETED 1
#define MF_READ 2
#define MF_NEW 4

// External declarations

// Local globals
int mdb_alloc;
int mdb_top;
int mdb_first_free;

typedef int mdbref;

// Global structures (should be placed into header file -- Belisarius)
struct mdb_entry
{
    dbref    from;

    long     date;
    long    rdate;
    int     flags;          // see the following.

    char *message;          // null if unused entry

    mdbref   next;          // next garbage, or next message.
} *mdb;

static __inline__ mdbref get_mailk(dbref player)
{
    char *i;

    if (!*(i = atr_get(player, A_MAILK)))
    {
        return NOMAIL;
    }
    else
    {
        return atoi(i);
    }
}

static __inline__ void set_mailk(dbref player, mdbref mailk)
{
    char buf[20];
    sprintf(buf, "%d", mailk);
    atr_add(player, A_MAILK, buf);
}

void check_mail(dbref player)
{
    char buf[BUF_SIZE];
    int read = 0;
    int new  = 0;
    int total  = 0;
    mdbref mail_db_obj;

    if (get_mailk(player) == NOMAIL)
    {
        return;
    }

    for (mail_db_obj = get_mailk(player); mail_db_obj != NOMAIL; mail_db_obj=mdb[mail_db_obj].next)
    {
        if (mdb[mail_db_obj].flags & MF_READ)
        {
            read++;
        }

        if (mdb[mail_db_obj].flags & MF_NEW)
        {
            new++;
        }

        if (!(mdb[mail_db_obj].flags & MF_DELETED))
        {
            total++;
        }
    }

    sprintf(buf, "You have %d message%s.", total, (total == 1) ? "": "s");

    if (new)
    {
        sprintf(buf+strlen(buf), " %d of them %s new.", new, (new == 1) ? "is" : "are");

        if ((total-read-new) > NONE)
        {
            sprintf(buf+strlen(buf)-1, "; %d other%s unread.", total-read-new, (total-read-new == 1)?" is":"s are");
        }
    }
    else
    {
        if ((total-read) > NONE)
        {
            sprintf(buf+strlen(buf), " %d of them %s unread.", total-read, (total-read == 1) ? "is" : "are");
        }
    }

    send_message(player, buf);
}

static mdbref grab_free_mail_slot()
{
    if (mdb_first_free != NOMAIL)
    {
        if (mdb[mdb_first_free].message)
        {
            log_error("+mail's first_free's message isn't null!");
            mdb_first_free = NOMAIL;
        }
        else
        {
            mdbref z = mdb_first_free;
            mdb_first_free = mdb[mdb_first_free].next;
            return z;
        }
    }

    if (++mdb_top >= mdb_alloc)
    {
        mdb_alloc *= 2;
        mdb = realloc(mdb, sizeof(struct mdb_entry) * mdb_alloc);
    }

    mdb[mdb_top-1].message = NULL;

    return mdb_top - 1;
}

static void make_free_mail_slot(mdbref mail_obj_id)
{
    if (mdb[mail_obj_id].message)
    {
        free(mdb[mail_obj_id].message);
    }

    mdb[mail_obj_id].message = NULL;
    mdb[mail_obj_id].next = mdb_first_free;
    mdb_first_free = mail_obj_id;
}

void init_mail()
{
    mdb_top = 0;
    mdb_alloc = 512;
    mdb = malloc(sizeof(struct mdb_entry)*mdb_alloc);
    mdb_first_free = NOMAIL;
}

void free_mail()
{
    for (int mail_obj_id = 0; mail_obj_id < mdb_top; mail_obj_id++)
    {
        if (mdb[mail_obj_id].message)
        {
            free(mdb[mail_obj_id].message);
        }
    }

    if (mdb)
    {
        free(mdb);
    }
}

static void send_mail_as(dbref from, dbref recip, char *message, time_t when, time_t rdate, int flags)
{
    mdbref prev = NOMAIL;
    mdbref mail_obj_id;
    int msgno = 1;

    for (mail_obj_id = get_mailk(recip); mail_obj_id != NOMAIL; mail_obj_id = mdb[mail_obj_id].next)
    {
        if (mdb[mail_obj_id].flags&MF_DELETED)
        {
            // we found a spot! 
            break;
        }
        else
        {
            prev=mail_obj_id;
            msgno++;
        }
    }

    if (mail_obj_id == NOMAIL)
    {
        // sigh. no deleted messages to fill in. we'll have to tack a 
        // new one on the end. 
        if (prev == NOMAIL)
        {
            // they've got no mail at all. 
            mail_obj_id = grab_free_mail_slot();

            set_mailk(recip,mail_obj_id);
        }
        else
        {
            mdb[prev].next = mail_obj_id = grab_free_mail_slot();
        }

        mdb[mail_obj_id].next = NOMAIL;
    }

    mdb[mail_obj_id].from = from;
    mdb[mail_obj_id].date = when;
    mdb[mail_obj_id].rdate = rdate;
    mdb[mail_obj_id].flags = flags;
    SET(mdb[mail_obj_id].message, message);

    if (from != NOTHING)
    {
        send_message(recip, "You sense that you have new mail from %s (message number %d)",unparse_object(recip,from),msgno);
    }
    else
    {
        send_message(recip, "You sense that you have new mail (message number %d).",msgno);
    }
}

///////////////////////////////////////////////////////////////////////////////
// send_mail()
///////////////////////////////////////////////////////////////////////////////
//    This is a wrapper function for send_mail_as() and assumes the message is
// being sent in real time.
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
static void send_mail(dbref from, dbref recip, char *message)
{
    send_mail_as(from, recip, message, now, now, MF_NEW);
}

int dt_mail(dbref who)
{
    int count = 0;

    // Not a player?
    if (Typeof(who) != TYPE_PLAYER)
    {
        // Only real players get mail
        return NOTHING;
    }

    // Loop through the mail and rack up the count
    for (mdbref mail_obj_id = get_mailk(who); mail_obj_id != NOMAIL; mail_obj_id = mdb[mail_obj_id].next)
    {
        count++;
    }

    return count;
}

void do_mail(dbref player, char *arg1, char *arg2)
{
    char buf[MAX_BUF];

    // Check to see if its a registered player
    if (Typeof(player) != TYPE_PLAYER || Guest(player))
    {
        send_message(player, "Sorry, only real players can use mail.");
        return;
    }

    // Check to see which mail command was used
    if (!string_compare(arg1, "delete"))
    {
        // delete a message
        mdbref i;
        int num;

        if (*arg2)
        {
            num = atoi(arg2);

            if (num <= 0)
            {
              	send_message(player, "You have no such message.");
            	return;
            }

            for (i = get_mailk(player); i != NOMAIL && 0 < --num; i = mdb[i].next);

            if (i == NOMAIL)
            {
                send_message(player, "You have no such message.");
            }
            else
            {
                mdb[i].flags = MF_DELETED;
                send_message(player, "Ok, deleted.");
            }
        }
        else
        {
            for (i = get_mailk(player); i != NOMAIL; i = mdb[i].next)
            {
                mdb[i].flags = MF_DELETED;
            }

            send_message(player, "All messages deleted.");
        }
    }
    else if (!string_compare(arg1,"undelete"))
    {
        // delete a message.
        mdbref i;
        int num;

        if (*arg2)
        {
            num = atoi(arg2);

            if (num <= 0)
            {
                send_message(player, "You have no such message.");
                return;
            }

            for (i = get_mailk(player); i != NOMAIL && 0 < --num; i = mdb[i].next)
            ;

            if (i == NOMAIL)
            {
                send_message(player, "You have no such message.");
            }
            else
            {
              	mdb[i].flags = MF_READ;
          	send_message(player, "Ok, undeleted.");
            }
        }
        else
        {
            for (i = get_mailk(player); i != NOMAIL; i = mdb[i].next)
            {
          	mdb[i].flags = MF_READ;
            }

            send_message(player, "All messages undeleted.");
        }
    }
    else if (!string_compare(arg1, "purge"))
    {
        // purge deleted messages. 
        mdbref i;
        mdbref next;
        mdbref prev = NOMAIL;

        for (i = get_mailk(player); i != NOMAIL; i = next)
        {
            next = mdb[i].next;

            if (mdb[i].flags&MF_DELETED)
            {
                if (prev != NOMAIL)
                {
                    mdb[prev].next = mdb[i].next;
                }
                else
                {
                    set_mailk(player,mdb[i].next);
                    make_free_mail_slot(i);
                }
            }
            else
            {
                prev = i;
            }
        }

        send_message(player,"deleted messages purged.");
    }
    else if (!strcmp(arg1, "Clear"))
    {
        // delete and purge _all_ messages! 
        mdbref i, next;
        mdbref prev = NOMAIL;

        for (i = get_mailk(player); i != NOMAIL; i = mdb[i].next)
        {
            mdb[i].flags = MF_DELETED;
        }

        for (i = get_mailk(player); i != NOMAIL; i = next)
        {
            next = mdb[i].next;

            if (mdb[i].flags&MF_DELETED)
            {
                if (prev != NOMAIL)
                {
                    mdb[prev].next = mdb[i].next;
                }
                else
                {
                    set_mailk(player,mdb[i].next);
                }

                make_free_mail_slot(i);
            }
            else
            {
                prev = i;
            }
        }
        send_message(player, "All messages deleted and purged.");
    }
    else if (*arg1 && !*arg2)
    {
        // must be reading a message. 
        int i, k;
        mdbref j = NOMAIL;
        char buf[BUF_SIZE];

        k = i = atoi(arg1);

        if (i > 0)
        {
              for (j = get_mailk(player); j != NOMAIL && i > 1; j = mdb[j].next, i--) ;
        }

        if (j == NOMAIL)
        {
              send_message(player, "Invalid message number.");
              return;
        }

        send_message(player, "Message %d:", k);
        send_message(player, "From: %s", (mdb[j].from != NOTHING) ? unparse_object(player, mdb[j].from) : "The MUSE server");
        sprintf(buf, "Date: %s", mktm(mdb[j].date, "D", player));
        send_message(player, buf);

        if (mdb[j].flags & MF_READ)
        {
              sprintf(buf, "Read at: %s", mktm(mdb[j].rdate, "D", player));
              send_message(player, buf);
        }

        strcpy(buf, "Flags:");

        if (mdb[j].flags & MF_DELETED) strcat(buf, " deleted");
        if (mdb[j].flags & MF_READ)    strcat(buf, " read");
        if (mdb[j].flags & MF_NEW)     strcat(buf, " new");

        send_message(player, buf);

        if (power(player, POW_SECURITY))
        {
              send_message(player, "Mailk: %d", j);
        }

        send_message(player, "");
        send_message(player, mdb[j].message);

        if (mdb[j].flags & MF_NEW)
        {
              mdb[j].rdate = now;
        }

        mdb[j].flags &=~MF_NEW;
        mdb[j].flags |= MF_READ;
    }
    else if (!*arg1 && !*arg2)
    {
        // list mail.
        mdbref j;
        int i=1;
        char buf[BUF_SIZE];

        for (j = get_mailk(player); j != NOMAIL; j = mdb[j].next, i++)
        {
            char status = 'u';

            if (mdb[j].flags & MF_DELETED)
            {
                status='d';
            }
            else if (mdb[j].flags & MF_NEW)
            {
                status='*';
                mdb[j].flags &= ~MF_NEW;
            }
            else if (mdb[j].flags & MF_READ)
            {
                status=' ';
            }

            sprintf(buf, "%3d) %c %s %s", i, status, unparse_object(player, mdb[j].from), mktm(mdb[j].date, "D", player));
            send_message(player, buf);
        }

        send_message(player, "");
    }
    else if (!*arg1 && *arg2)
    {
        send_message(player, "You want to do what?");
    }
    else if (*arg1 && *arg2)
    {
        // send mail 
        dbref recip;

        recip = lookup_player(arg1);

        if (recip == NOTHING || Typeof(recip) != TYPE_PLAYER)
        {
            send_message(player, "i haven't a clue who you're talking about.");
            return;
        }

        if (!could_doit(player, recip, A_LPAGE))
        {
            send_message(player, "That player is page-locked against you.  Sorry.");
            return;
        }

        send_mail(player, recip, arg2);
        send_message(player, "You mailed %s with '%s'", unparse_object(player, recip), arg2);
    }
    else
    {
        sprintf(buf, "We shouldn't get here +mail. arg1: %s. arg2: %s.", arg1, arg2);
        log_error(buf);
    }
}

void write_mail(FILE *file)
{
    mdbref mail_obj;

    // Loop through every database object
    for (dbref db_obj_id = 0; db_obj_id < db_top; db_obj_id++)
    {
        if (Typeof(db_obj_id) == TYPE_PLAYER && (mail_obj = get_mailk(db_obj_id)) != NOMAIL)
        {
            for (; mail_obj != NOMAIL; mail_obj = mdb[mail_obj].next)
            {
                if (!(mdb[mail_obj].flags & MF_DELETED))
                {
                    fprintf(file, "+%d:%d:%ld:%ld:%d:%s\n", mdb[mail_obj].from, db_obj_id, mdb[mail_obj].date, mdb[mail_obj].rdate, mdb[mail_obj].flags, mdb[mail_obj].message);
                }
            }
        }
    }
}

void read_mail(FILE *file)
{
    char buf[BUF_SIZE*2];
    dbref to;
    dbref from;
    time_t date;
    time_t rdate;
    int flags;
    char message[BUF_SIZE];
    char *s;


    while (fgets(buf, BUF_SIZE*2, file))
    {
        if (*buf == '+')
        {
            from = atoi(s = buf+1);

            if ((s = strchr(buf, ':')))
            {
          	to = atoi(++s);

          	if ((s=strchr(s, ':')))
                {
                    date = atoi(++s);

                    if ((s = strchr(s, ':')))
                    {
                        rdate = atoi(++s);
                    }

      	            if ((s = strchr(s, ':')))
                    {
                        flags = atoi(++s);

                        if ((s = strchr(s, ':')))
                        {
                            strcpy (message, ++s);
                            if ((s = strchr(message,'\n')))
                            {
                                *s = '\0';
                                // a good one. we just ignore bad ones. 
                                send_mail_as(from, to, message, date, rdate, flags);
                            }
                        }
                    }
                }
            }
        }
    }
}

void clear_old_mail()
{
    char buf[BUF_SIZE];
    dbref user;
    mdbref mail;
    int num_msgs;

    // Clears old read mail from the database, using the value set in 
    // config.c as the age of the mail.  If the age is 0, nothing is deleted
    // at all.

    if (max_mail_age == 0)
    {
        return;
    }

    for (user = 0; user < db_top; user++)
    {
        if (Typeof(user) == TYPE_PLAYER && (mail = get_mailk(user)) != NOMAIL)
        {
            num_msgs = 0;
            for (; mail != NOMAIL; mail = mdb[mail].next)
            {
                if (mdb[mail].flags & MF_READ)
                {
                    time_t diff = now - mdb[mail].rdate;

                    if (diff >= max_mail_age)
                    {
                        mdb[mail].flags = MF_DELETED;
                        num_msgs++;
                    }
                }
            }

            if (num_msgs > NONE)
            {
                mdbref i;
                mdbref next;
                mdbref prev = NOMAIL;

                if (num_msgs == 1)
                {
                    send_message(user, "1 of your old, read +mail messages was deleted.");
                    sprintf(buf, "Cleared 1 message from %s", unparse_object(root, user));
                    log_io(buf);
                }
                else
                {
                    send_message(user, "%d of your old, read +mail messages were deleted.", num_msgs);
                    sprintf(buf, "Cleared %d messages from %s", num_msgs, unparse_object(root, user));
                    log_io(buf);
                }

              	// purge deleted messages.
                for (i = get_mailk(user); i != NOMAIL; i = next)
                {
                    next = mdb[i].next;

                    if (mdb[i].flags&MF_DELETED)
                    {
                        if (prev != NOMAIL)
                        {
                            mdb[prev].next = mdb[i].next;
                        }
                        else
                        {
                            set_mailk(user,mdb[i].next);
                        }

                        make_free_mail_slot(i);
                    }
                    else
                    {
                        prev = i;
                    }
                }
            }
        }
    }
}
