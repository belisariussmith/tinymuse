// newconc.c 
// $Id: newconc.c,v 1.5 1993/08/22 04:54:13 nils Exp $ 

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "tinymuse.h"
#include "db.h"
#include "net.h"
#include "externs.h"

static long spot = 0;        // noone's gunna be on for 68 years, so
                             // we can have this wrap around unless
                             // people log on more often than 1/second.
                             //

long make_concid()
{
    spot++;

    if (spot < 0)
    {
        spot = 1;
    }

    return spot;
}

struct conc_list
{
    char *ip;
    char *pass;
    char used;
} concs[]={
  {"127.0.0.1", "foogarble", 0}
};

int numconcs = sizeof(concs) / sizeof(struct conc_list);

static int can_be_a_conc(struct sockaddr_in *addr, char *pass)
{
    int k;

    for (k = 0; k < numconcs; k++)
    {
        if (!strcmp(pass, concs[k].pass))
        {
            if (addr->sin_addr.s_addr == inet_addr(concs[k].ip))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

void do_makeid(struct descriptor_data *d)
{
    char buf[BUF_SIZE];

    if (!(d->cstatus&C_CCONTROL))
    {
        queue_string(d,"but.. but.. you're not a concentrator!\r\n");
        return;
    }

    sprintf(buf, "//Here's a new concentrator ID: %ld\n", make_concid());
    queue_string(d, buf);
}

void do_becomeconc(struct descriptor_data *d, char *pass)
{
    if (d->cstatus & C_CCONTROL)
    {
        queue_string(d, "but.. but.. you're already a concentrator!\r\n");
        return;
    }

    if (can_be_a_conc(&d->address,pass))
    {
        d->cstatus |= C_CCONTROL;
        queue_string(d, "//Welcome to the realm of concentrators.\r\n");
    }
    else
    {
        queue_string(d, "but.. but.. i can't let you in with that passwd and/or host.\r\n");
    }
}

void do_connectid(struct descriptor_data *d, long concid, char *addr)
{
    struct descriptor_data *k;

    puts("here1");

    for (k = descriptor_list; k; k = k->next)
    {
        if(k->concid == concid)
        {
            queue_string(d, "//Sorry, there's already someone with that concid.\r\n");
            return;
        }
    }

    k = malloc(sizeof(struct descriptor_data));

    k->descriptor    = d->descriptor;
    k->concid        = concid;
    k->cstatus       = C_REMOTE;
    k->parent        = d;
    k->state         = WAITCONNECT;
    k->output_prefix = 0;
    k->output_suffix = 0;
    k->output_size   = 0;
    k->output.head   = 0;
    k->output.tail   = &k->output.head;
    k->input.head    = 0;
    k->input.tail    = &k->input.head;
    k->raw_input     = 0;
    k->raw_input_at  = 0;
    k->quota         = command_burst_size;
    k->last_time     = 0;

    strcpy(k->addr,addr);
    k->address = d->address;

    if (descriptor_list)
    {
        descriptor_list->prev = &k->next;
    }

    k->next = descriptor_list;
    k->prev = &descriptor_list;

    descriptor_list = k;

    welcome_user (k);
}

void do_killid(struct descriptor_data *d, long id)
{
    struct descriptor_data *k;

    if (id == d->concid)
    {
        queue_string(d, "what in the world are you trying to do?\r\n");
        return;
    }

    for(k = descriptor_list; k; k = k->next)
    {
        if(k->concid == id)
        {

            if(k->descriptor != d->descriptor)
            {
                queue_string(d, "don't do that. that's someone else's.\r\n");
                return;
            }
            else
            {
                shutdownsock(k);
                return;
            }
        }
    }
}
