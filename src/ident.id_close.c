//
// id_close.c                            Close a connection to an IDENT server
//
// Author: Peter Eriksson <pen@lysator.liu.se>
//
#include "tinymuse.h"

#define IN_LIBIDENT_SRC
#include "ident.h"

int id_close(ident_t *id)
{
    int res;

    res = close(id->fd);
    free(id);

    return res;
}
