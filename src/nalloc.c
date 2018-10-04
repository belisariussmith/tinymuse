///////////////////////////////////////////////////////////////////////////////
// nalloc.c                                              TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// Modified nalloc.c provided by Rick S. Harby (Tabok), prevents access to
// memory already freed like the original nalloc.c did. Also logs when it
// frees old glurp space. Has the ability to shrink the nalloc space back
// to the default_glurp_size after a given number of commands
///////////////////////////////////////////////////////////////////////////////
#include "tinymuse.h"
#include "externs.h"

#define DEFAULT_GLURP_SIZE 8192

#define MAX_HITS	10

void *glurp = NULL;
int glurpsize = 0;
int glurppos = 0;

struct oldglurp
{
  struct oldglurp *next;
} *olds = NULL;

static void biggerglurp(int line, char *file)
{
    struct oldglurp *o;
    char buf[200];

    void *oldglurp = NULL;

    if (!glurp)
    {
        glurp     = malloc(DEFAULT_GLURP_SIZE);
        glurpsize = DEFAULT_GLURP_SIZE;
        glurppos  = 0;

        // Do these redundant because of the glurps done by log_io 

        o        = newglurp(sizeof(struct oldglurp));
        o->next  = NULL;
        olds     = o;

        sprintf(buf, "Initial glurp size set to %d (%s:%d)", glurpsize, file, line);
        log_io(buf);
    }
    else
    {
        glurpsize *= 2;
        glurppos   = 0;

        oldglurp = glurp;
        glurp    = malloc(glurpsize);

        // Do these redundant because of the glurps done by log_io 

        o       = newglurp(sizeof(struct oldglurp));
        o->next = oldglurp;
        olds    = o;

        sprintf(buf, "doubling glurp size to %d (%s:%d)", glurpsize, file, line);
        log_io(buf);
    }
}

void *newglurp_fil(int size, int line, char *file)
{
    size = (size+7)/8;

    if (size <= 0)
    {
        size = 1;
    }

    size *= 8;

    while (glurppos+size > glurpsize)
    {
        biggerglurp(line, file);
    }

    glurppos += size;

    return (void *)(((char *)glurp) + (glurppos-size));
}

void free_old_glurps(struct oldglurp *ptr, int size)
{
    char buff[200];

    if (ptr)
    {
        free_old_glurps((struct oldglurp *)ptr->next, size / 2);
        sprintf(buff, "freeing old glurp space: Size %d", size);
        log_io(buff);

        free((char *)ptr);
    }
}

///////////////////////////////////////////////////////////////////////////////
// clearglurp()
///////////////////////////////////////////////////////////////////////////////
//     Shrinks glurp size
///////////////////////////////////////////////////////////////////////////////
// Returns: -
///////////////////////////////////////////////////////////////////////////////
void clearglurp()
{
    static int hitcount = 0;

    if (olds && olds->next && (glurpsize > DEFAULT_GLURP_SIZE))
    {
        free_old_glurps((struct oldglurp *)olds->next, glurpsize / 2);
        olds->next = NULL;
    }

    if ((glurpsize > DEFAULT_GLURP_SIZE) && ((glurpsize / 2) > glurppos))
    {
        if (++hitcount > MAX_HITS)
        {
            struct oldglurp *o;
            char buff[200];

            sprintf(buff, "shrinking glurp space from %d to %d", glurpsize, glurpsize / 2);
            log_io(buff);
            free(glurp);

            glurpsize /= 2;
            glurp      = malloc(glurpsize);
            glurppos   = 0;
            o          = newglurp(sizeof(struct oldglurp));
            o->next    = NULL;
            olds       = o;
            hitcount   = 0;
        }
    }

    glurppos = 0 + sizeof(struct oldglurp);	// that was easy.
}

char *glurpdup(char *s)
{
    char *op;

    op = newglurp(strlen(s)+1);
    strcpy(op, s);

    return op;
}
