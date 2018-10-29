///////////////////////////////////////////////////////////////////////////////
// utils.c                                                TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// $Id: utils.c,v 1.4 1993/08/16 01:57:27 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include <sys/types.h>
#include <time.h>
#include <ctype.h>

#include "tinymuse.h"
#include "externs.h"
#include "db.h"
#include "config.h"

static long get_timezone()
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv,&tz);

    return tz.tz_minuteswest * 60;
}

dbref find_entrance(dbref door)
{
    return db[door].location;
}

// remove the first occurence of what in list headed by first 
dbref remove_first(dbref first, dbref what)
{
    dbref prev;
  
    // special case if it's the first one 
    if (first == what)
    {
        return db[first].next;
    }
    else
    {
        // have to find it 
        DOLIST(prev, first)
        {
            if (db[prev].next == what)
            {
                db[prev].next = db[what].next;
                return first;
            }
        }

        return first;
    }
}

int member(dbref thing, dbref list)
{
    DOLIST(list, list)
    {
        if (list == thing)
        {
            return TRUE;
        }
    }
  
    return FALSE;
}

dbref reverse(dbref list)
{
    dbref newlist;
    dbref rest;
  
    newlist = NOTHING;

    while(list != NOTHING)
    {
        rest = db[list].next;
        PUSH(list, newlist);
        list = rest;
    }

    return newlist;
}

static void get_tz(char *tz, char **tzp, int *lyp)
{
    char *s;

    *lyp = 0;
    *tzp = tz;

    if ( *tzp != NULL )
    {
        if ( **tzp != '\0' )
        {
            if ( (s = strchr(*tzp, ':')) != NULL )
            {
                ++s;
                if ( *s == 'Y' || *s == 'y' )
                {
                    *lyp = 1;
                }
            }
        }
    }
}

static void get_thing_tz(dbref thing, char **tzp, int *lyp)
{
    char *s;

    *tzp = atr_get(thing, A_TZ);

    if ( **tzp == '\0' )
    {
        *tzp = atr_get(db[thing].owner, A_TZ);
    }

    if ( **tzp == '\0' )
    {
        *lyp = 0;
    }
    else if ( (s = strchr(*tzp, ':')) == NULL )
    {
        *lyp = 0;
    }
    else
    {
        ++s;

        if ( *s == 'Y' || *s == 'y' )
        {
            *lyp = 1;
        }
    }
}

char *mktm(time_t cl, char *tz, dbref thing)
{
    int ly;
    char *s;
    struct tm *tmp;
    long adjust, utcdiff;

    if ( tz == NULL )
    {
        tz = "";
    }
    if ( *tz == 'D' || *tz == '\0' )
    {
        get_thing_tz(thing, &tz, &ly);
    }
    else
    {
        get_tz(tz, &tz, &ly);
    }

    // determine time diff between gmt and local standard time 
    utcdiff = get_timezone();
    tmp = localtime(&cl);

    if ( tmp )
    {
        if ( tmp->tm_isdst )
        {
            utcdiff -= 3600L;
        }
    }

    // get or calculate timezone adjustment 
    if ( *tz == '\0' )
    {
        adjust = utcdiff;
    }
    else
    {
        adjust = 0L - atol(tz) * 3600L;
        if ( ly && tmp->tm_isdst > 0 )
            adjust -= 3600L;
    }

    // adjust for timezone 
    cl += (utcdiff - adjust);

    // generate ascii string 
    s = ctime(&cl);
    s[strlen(s)-1]='\0';

    return s;
}

// this routine is designed to interpret a wide variety of time string
// formats converting them all to a longint x-value time representation */
long mkxtime(char *s, dbref thing, char *tz)
{
    char *p, *q = s, *dayp, *yearp = NULL;
    struct tm tmbuf;
    long utcdiff;
    long adjust;
    long cl;
    int ly;
    int i;
    int j;
    int seconds, minutes, hours, day, month, year;

    static int tadjust[] = {
         0,    -4,    -5,    -5,    -6,    -6,    -7,    -7,    -8
    };
    static char *tname[] = {
        "gmt", "edt", "est", "cdt", "cst", "mdt", "mst", "pdt", "pst", NULL
    };
    static char *mname[] = {
        "jan", "feb", "mar", "apr", "may", "jun",
        "jul", "aug", "sep", "oct", "nov", "dec", NULL
    };

    // Locate month if present 
    month = -1;

    for ( p = s; month < 0 && *p != '\0'; p++ )
    {
        if ( isalpha(*p) )
        {
            for ( i = 0; mname[i] != NULL; i++ )
            {
                j = 0, q = p;

                while ( mname[i][j] != '\0' && mname[i][j] == to_lower(*q) )
                {
                    ++j, ++q;
                }

                if ( mname[i][j] == '\0' )
                {
                    month = i;
                    break;
                }
            }
        }
    }

    if ( month < 0 )
    {
        return -1L;
    }

    // skip remaining string and whitespace 
    for ( p = q; isalpha(*p); p++ )
    ;
    for ( ; isspace(*p); p++ )
    ;

    // get day 
    day = -1;

    if ( isdigit(*p) )
    {
        day = atoi(p);
    }

    if ( day < 1 || day > 31)
    {
        return -1L;
    }

    dayp = p;

    // get year 
    year = -1;

    for ( p = s; *p != '\0'; p++ )
    {
        if ( isdigit(*p) )
        {
            year = atoi(p);

            if ( year < 1900 )
            {
                year = -1;
                for ( ; isdigit(*p); p++ )
                ;
                continue;
            }

            yearp = p;
            break;
        }
    }

    if ( year < 0 )
    {
        return -1L;
    }

    // get hours 
    hours = -1;

    for ( p = s; *p != '\0'; p++ )
    {
        if ( isdigit(*p) )
        {
            hours = atoi(p);

            if ( p == dayp || p == yearp || hours < 0 || hours > 59 )
            {
                hours = -1;

                for ( ; isdigit(*p); p++ )
                ;

                continue;
            }
            break;
        }
    }

    if ( hours < 0 )
    {
        return -1L;
    }

    for ( ; isdigit(*p); p++ )
    ;

    // get minutes 
    minutes = -1;

    if ( *p == ':' )
    {
        minutes = atoi(++p);
    }

    if ( minutes < 0 || minutes > 59 )
    {
        return -1L;
    }

    for ( ; isdigit(*p); p++ )
    ;

    // get seconds 
    seconds = -1;

    if ( *p == ':' )
    {
        seconds = atoi(++p);
    }

    if ( seconds < 0 || seconds > 59 )
    {
        return -1L;
    }

    tmbuf.tm_sec   = seconds;
    tmbuf.tm_min   = minutes;
    tmbuf.tm_hour  = hours;
    tmbuf.tm_mday  = day;
    tmbuf.tm_mon   = month;
    tmbuf.tm_year  = year - 1900;
    tmbuf.tm_isdst = -1;
    cl = mktime(&tmbuf);

    // determine time diff between gmt and local standard time 
    // (for the time calculated) 
    utcdiff = ( tmbuf.tm_isdst > 0 ) ? get_timezone() - 3600L : get_timezone();

    // calculate timezone adjustment 
    if ( tz == NULL )
    {
        tz = "";
    }

    if ( *tz == 'D' || *tz == '\0' )
    {
        get_thing_tz(thing, &tz, &ly);
    }
    else
    {
        get_tz(tz, &tz, &ly);
    }

    if ( *tz == '\0' )
    {
        adjust = utcdiff;
    }
    else
    {
        adjust = 0L - atol(tz) * 3600L;
        if ( ly && tmbuf.tm_isdst > 0 )
        adjust -= 3600L;
    }

    // check for timezone override in string specification 
    for ( p = s; *p != '\0'; p++ )
    {
        if ( isalpha(*p) )
        {
            for ( i = 0; tname[i] != NULL; i++ )
            {
                j = 0; q = p;
                while ( tname[i][j] != '\0' && tname[i][j] == to_lower(*q) )
                    ++j, ++q;
                if ( tname[i][j] == '\0' )
                {
                    adjust = 0L - (3600L * (long) (tadjust[i]));
                    goto OUT;
                }
            }
        }
    }

    OUT:

    cl += (adjust - utcdiff);

    return cl;
}
