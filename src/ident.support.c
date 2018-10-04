#include "tinymuse.h"
#include <stdio.h>
#include <ctype.h>
#define IN_LIBIDENT_SRC
#include "ident.h"

char *_id_strdup(char * str)
{
    char *cp;

    cp = (char *) malloc(strlen(str)+1);
    strcpy(cp, str);

    return cp;
}


///////////////////////////////////////////////////////////////////////////////
// _id_strtok()
///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
// Returns: Boolean 
///////////////////////////////////////////////////////////////////////////////
char* _id_strtok(char * cp, char *cs, char *dc)
{
    static char *bp = FALSE;
    
    if (cp)
    {
        bp = cp;
    }
    
    // No delimitor cs - return whole buffer and point at end
    if (!cs)
    {
	while (*bp)
        {
	    bp++;
        }
	return cs;
    }
    
    // Skip leading spaces
    while (isspace(*bp))
    {
	bp++;
    }
    
    // No token found?
    if (!*bp)
    {
	return FALSE;
    }
    
    cp = bp;
    while (*bp && !strchr(cs, *bp))
	bp++;
    
    // Remove trailing spaces
    *dc = *bp;

    for (dc = bp-1; dc > cp && isspace(*dc); dc--)
	;

    *++dc = '\0';
    
    bp++;
    
    return cp;
}
