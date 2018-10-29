///////////////////////////////////////////////////////////////////////////////
// ansi.c                                                  TinyMUSE (@)
///////////////////////////////////////////////////////////////////////////////
// To make the MUD colorful using ANSI colod codes.
///////////////////////////////////////////////////////////////////////////////
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#include "tinymuse.h"
#include "interface.h"
#include "externs.h"

#define COLOR_CHAR	'|'

typedef struct color_def {
	char *name;
	int  code;
} colortype;

colortype colors[]={
	{"NORMAL",		0},
	{"BRIGHT",		1},
	{"BLINK",		5},
	{"REVERSE",		7},
	{"BLACK",		30},
	{"RED",			31},
	{"GREEN",		32},
	{"YELLOW",		33},
	{"BLUE",		34},
	{"MAGENTA",		35},
	{"CYAN",		36},
	{"WHITE",		37},
	{"BLACKB",		40},
	{"REDB",		41},
	{"GREENB",		42},
	{"YELLOWB",		43},
	{"BLUEB",		44},
	{"MAGENTAB",		45},
	{"CYANB",		46},
	{NULL,			-1}};

char color_string[BUF_SIZE];

int get_color(char *str)
{
    int i;

    for (i = 0; colors[i].name; i++)
    {
        if (!strcmp(colors[i].name, str))
        {
            return colors[i].code;
        }
    }

    return -1;
}

char *translate_color(char *str, int add)
{
    static char buffer[BUF_SIZE];
    char *newstr = NULL;
    char *p;
    int c;

    SET(newstr,str);

    if (add)
    {
        strcpy(buffer, "[");
    }
    else
    {
        strcpy(buffer, "");
    }

    while ((p=parse_up(&newstr,',')))
    {
        while(p && *p && isspace(*p))
        {
            p++;
        }

        if ((c = get_color(p)) != -1)
        {
            if (add)
            {
                if (c == 0)
                {
                    sprintf(buffer, "%s;37;%d", buffer, c);
                }
                else
                {
                    sprintf(buffer, "%s;%d", buffer, c);
                }
            }
        }
        else
        {
            sprintf(buffer, "%c%s%c", COLOR_CHAR, str, COLOR_CHAR);
            return buffer; 
        }
    }

    if (add)
    {
        sprintf(buffer, "%sm", buffer);
    }

    return buffer;
}

int is_color(char *str)
{
    char buffer[MAX_CMD_BUF];
    char *tmpstr = NULL, *p = NULL;

    strcpy(buffer, str);
    tmpstr = buffer;

    if (!(p=strchr(tmpstr, COLOR_CHAR)))
    {
        return FALSE;
    }

    if (p == tmpstr)
    {
        return FALSE;
    }

    *p='\0';

    while((p = parse_up(&tmpstr, ',')))
    {
        if (get_color(p) == -1)
        {
            return FALSE;
        }
    }

    return TRUE;
}

char *addcolor(char **strptr, int flag)
{
    char *p = parse_up(strptr, COLOR_CHAR);

    return translate_color(p, flag);
}

char *color(char *str, int flag)
{
    static char buffer[MAX_CMD_BUF], buffer2[MAX_CMD_BUF];
    char *newstr = NULL, *p = NULL, *k = NULL;

    strcpy(buffer, "");
    strcpy(buffer2, str);

    newstr = buffer2;
    k      = newstr;

    for (p = strchr(newstr, COLOR_CHAR); p; p = strchr(p, COLOR_CHAR))
    {
        if (is_color(p + 1))
        {
            *p++='\0';
            sprintf(buffer, "%s%s%s", buffer, k, addcolor(&p, flag));
            k = p;
        }
        else
        {
            p++;
        }
    }

    sprintf(buffer, "%s%s", buffer, k);

    return buffer;
}
