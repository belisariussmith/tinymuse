//
// id_open.c                 Establish/initiate a connection to an IDENT server
//
// Author: Peter Eriksson <pen@lysator.liu.se>
// Fixes: P�r Emanuelsson <pell@lysator.liu.se>
//
#include "tinymuse.h"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/file.h>
#include <netinet/in.h>

#define IN_LIBIDENT_SRC
#include "ident.h"

ident_t *id_open(struct in_addr *laddr, struct in_addr *faddr, struct timeval *timeout)
{
    ident_t *id;
    int res, tmperrno;
    struct sockaddr_in sin_laddr, sin_faddr;
    fd_set rs, ws, es;
    int on = 1;
    struct linger linger;

    if ((id = (ident_t *) malloc(sizeof(*id))) == 0)
    {
	return 0;
    }

    if ((id->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
	free(id);
	return 0;
    }

    if (timeout)
    {
	if ((res = fcntl(id->fd, F_GETFL, 0)) < 0)
        {
	    goto ERROR;
        }

	if (fcntl(id->fd, F_SETFL, res | FNDELAY) < 0)
        {
	    goto ERROR;
        }
    }

    // We silently ignore errors if we can't change LINGER 
    linger.l_onoff  = 0;
    linger.l_linger = 0;

    (void) setsockopt(id->fd, SOL_SOCKET, SO_LINGER, (void *) &linger, sizeof(linger));
    (void) setsockopt(id->fd, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on));

    id->buf[0] = '\0';

    bzero((char *)&sin_laddr, sizeof(sin_laddr));
    sin_laddr.sin_family = AF_INET;
    sin_laddr.sin_addr = *laddr;
    sin_laddr.sin_port = 0;

    if (bind(id->fd, (struct sockaddr *) &sin_laddr, sizeof(sin_laddr)) < 0)
    {
#ifdef DEBUG
	perror("libident: bind");
#endif
	goto ERROR;
    }

    bzero((char *)&sin_faddr, sizeof(sin_faddr));
    sin_faddr.sin_family = AF_INET;
    sin_faddr.sin_addr = *faddr;
    sin_faddr.sin_port = htons(IDPORT);

    errno = 0;
    res = connect(id->fd, (struct sockaddr *) &sin_faddr, sizeof(sin_faddr));

    if (res < 0 && errno != EINPROGRESS)
    {
#ifdef DEBUG
	perror("libident: connect");
#endif
	goto ERROR;
    }

    if (timeout)
    {
	FD_ZERO(&rs);
	FD_ZERO(&ws);
	FD_ZERO(&es);

	FD_SET(id->fd, &rs);
	FD_SET(id->fd, &ws);
	FD_SET(id->fd, &es);

	if ((res = select(FD_SETSIZE, &rs, &ws, &es, timeout)) < 0)
	{
#ifdef DEBUG
	    perror("libident: select");
#endif
	    goto ERROR;
	}

	if (res == 0)
	{
	    errno = ETIMEDOUT;
	    goto ERROR;
	}

	if (FD_ISSET(id->fd, &es))
        {
	    goto ERROR;
        }

	if (!FD_ISSET(id->fd, &rs) && !FD_ISSET(id->fd, &ws))
        {
	    goto ERROR;
        }
    }

    return id;

  ERROR:
    tmperrno = errno;		// Save, so close() won't erase it 
    close(id->fd);
    free(id);
    errno = tmperrno;

    return 0;
}
