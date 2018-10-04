//
// id_query.c                             Transmit a query to an IDENT server
//
// Author: Peter Eriksson <pen@lysator.liu.se>
//
#include "tinymuse.h"

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#define IN_LIBIDENT_SRC
#include "ident.h"

int id_query(ident_t *id, int lport, int fport, struct timeval *timeout)
{
    void (*old_sig) __P((int));
    int res;
    fd_set ws;

    char buf[80];

    sprintf(buf, "%d , %d\r\n", lport, fport);

    if (timeout)
    {
	FD_ZERO(&ws);
	FD_SET(id->fd, &ws);

	if ((res = select(FD_SETSIZE,
			  (fd_set *)0, &ws,
			  (fd_set *)0,
			  timeout)) < 0)
	    return -1;

	if (res == 0)
	{
	    errno = ETIMEDOUT;
	    return -1;
	}
    }

    old_sig = signal(SIGPIPE, SIG_IGN);

    res = write(id->fd, buf, strlen(buf));

    signal(SIGPIPE, old_sig);

    return res;
}
