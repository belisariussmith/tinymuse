///////////////////////////////////////////////////////////////////////////////
// wild.c                                                 TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: wild.c,v 1.3 1993/01/16 18:37:10 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
// wild card routine(s) created by Lawrence Foard 
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <ctype.h>

#include "tinymuse.h"
#include "config.h"
#include "externs.h"

// Local Globals
char wbuff[BUF_SIZE*2];
char *wptr[10];       
int wlen[10];

///////////////////////////////////////////////////////////////////////////////
// wild()
///////////////////////////////////////////////////////////////////////////////
//     int p;   - what argument are we on now? 
//     int os;  - true if just came from wild card state 
///////////////////////////////////////////////////////////////////////////////
// Return: 
///////////////////////////////////////////////////////////////////////////////
static int wild(char *s, char *d, int p, int os)
{
    switch(*s)
    {
        case '?':
            // match any character in d, note end of string is considered a match */
            // if just in nonwildcard state record location of change */
            if (!os && (p < 10))
            {
                wptr[p] = d; 
            }

            return (wild(s+1,(*d) ? d+1 : d,p,1));    
        case '*':
            // match a range of characters */
            if (s[1] == '*')
            {
                // double star. bad. -shkoo */
                return 0;
            }

            if (!os && (p < 10))
            {
                wptr[p] = d;
            }
            return (wild(s + 1, d, p, 1) || ((*d) ? wild(s, d + 1, p, 1) : 0));
        default:
            if (os && (p < 10))
            {
                wlen[p] = d - wptr[p];
                p++;
            }
            return ((to_upper(*s) != to_upper(*d)) ? 0 : ( (*s) ? wild(s + 1, d+ 1, p, 0) : 1));
    }
}

int wild_match(char *s, char *d)
{                      
    int a;
  
    for(a = 0; a < 10; a++)
    {
        wptr[a] = NULL;
    }
  
    switch(*s)
    {
        case '>':      
            s++;
            // if both first letters are #'s then numeric compare 
            if (isdigit(s[0]) || (*s=='-'))
            {
                return (atoi(s) < atoi(d));
            }
            else
            {
                return (strcmp(s, d) < 0);
            }
            break;
        case '<':
            s++;

            if (isdigit(s[0]) || (*s=='-'))
            {
                return(atoi(s) > atoi(d));
            }
            else
            {
                return(strcmp(s, d) > 0);
            }
            break;
        default:
            if (wild(s, d, 0, 0))
            {          
                int a, b;
                char *e, *f = wbuff;
      
                for (a = 0; a < 10; a++)
                {
                    if ((e = wptr[a]))
                    {
                        wptr[a] = f;

                        for (b = wlen[a]; b--; *f++ = *e++)
                        ;

                        *f++ = 0;
                    }
                }
                return(TRUE);
            }
            else
            {
                return(FALSE);
            }
    }

    return FALSE;
}        

