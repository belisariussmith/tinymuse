///////////////////////////////////////////////////////////////////////////////
// stringutil.c                                            TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: stringutil.c,v 1.3 1993/01/30 03:39:49 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
// String utilities 
///////////////////////////////////////////////////////////////////////////////
#include <ctype.h>

#include "tinymuse.h"
#include "db.h"
#include "config.h"
#include "externs.h"

#define DOWNCASE(x) to_lower(x)
///////////////////////////////////////////////////////////////////////////////
// string_compare()
///////////////////////////////////////////////////////////////////////////////
//     Case-insensitive string comparison.
///////////////////////////////////////////////////////////////////////////////
// Returns: int (0 for positive comparison) NOT BOOLEAN
///////////////////////////////////////////////////////////////////////////////
int string_compare(char *s1, char *s2)
{
    // Loop through both strings one character at a time
    while (*s1 && *s2 && DOWNCASE(*s1) == DOWNCASE(*s2))
    {
        // Characters both match, step to next character in each string
        s1++;
        s2++;
    }
  
    return(DOWNCASE(*s1) - DOWNCASE(*s2));
}

int string_prefix(char *string, char *prefix)
{
    while (*string && *prefix && DOWNCASE(*string) == DOWNCASE(*prefix))
    {
        string++;
        prefix++;
    }

    return *prefix == '\0';
}

// accepts only nonempty matches starting at the beginning of a word 
char *string_match(char *src, char *sub)
{
    if (*sub != '\0')
    {
        while (*src)
        {
            if (string_prefix(src, sub))
            {
                return src;
            }

            // else scan to beginning of next word 
            while(*src && isalnum(*src))
            {
                src++;
            }

            while(*src && !isalnum(*src))
            {
                src++;
            }
        }
    }
  
    return 0;
}

char to_upper(int x)
{
    if (x < 'a' || x > 'z')
    {
        return x;
    }

    return 'A' + (x - 'a');
}

char to_lower(int x)
{
    if (x < 'A' || x > 'Z')
    {
        return x;
    }

    return 'a' + (x - 'A');
}

char *str_index(char *what, int chr)
{
    char *x;

    for (x = what; *x; x++)
    {
        if (chr == *x)      
        {
            return x;
        }
    }

    return (char *)0;
}

char *int_to_str(int a)
{
    static char buf[100];

    if (a != 0)
    {
        sprintf(buf, "%d", a);
    }
    else
    {
        *buf='\0';
    }

    return buf;
}
