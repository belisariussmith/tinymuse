///////////////////////////////////////////////////////////////////////////////
// util/mkindx.c                                                      TinyMUSE
///////////////////////////////////////////////////////////////////////////////
// $Id: mkindx.c,v 1.3 1993/02/25 00:31:59 nils Exp $ 
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>

#include "tinymuse.h"
#include "externs.h"
#include "config.h"
#include "help.h"
  
char line[LINE_SIZE+1];

int main(int argc, char *argv[])
{
    long pos;
    int i, n, lineno, ntopics;
    char *s, *topic;
    help_indx entry;
    FILE *rfp, *wfp;
    char textfile[40], indxfile[40];
  
    if ( argc != 2 )
    {
        fprintf(stderr, "usage: mkindx <bundlename>\nfor instance: mkindx help\n");
        exit(1);
    }
  
    sprintf(textfile, "%stext", argv[1]);
    sprintf(indxfile, "%sindx", argv[1]);
  
    if ( (rfp = fopen(textfile, "r")) == NULL )
    {
        fprintf(stderr, "can't open %s for reading\n", textfile);
        exit(1);
    }

    if ( (wfp = fopen(indxfile, "w")) == NULL )
    {
        fprintf(stderr, "can't open %s for writing\n", indxfile);
        exit(1);
    }
  
    pos = 0L;
    lineno = 0;
    ntopics = 0;

    while ( fgets(line, LINE_SIZE, rfp) != NULL )
    {
        ++lineno;
    
        n = strlen(line);
        if ( line[n-1] != '\n' )  {
            fprintf(stderr, "line %d: line too long\n", lineno);
        }
    
        if ( line[0] == '&' )  {
            ++ntopics;
      
            if ( ntopics > 1 )  {
                entry.len = (int) (pos - entry.pos);
                if ( fwrite(&entry, sizeof(help_indx), 1, wfp) < 1 )  {
                    fprintf(stderr, "error writing %s\n", indxfile);
                    exit(2);
                }
            }
      
            for ( topic = &line[1]; (*topic == ' ' || *topic == '\t') && *topic != '\0'; topic++ )
            ;

            for ( i = -1, s = topic; *s != '\n' && *s != '\0'; s++ )
            {
                if ( i >= TOPIC_NAME_LEN-1 )
	            break;
	        if ( *s != ' ' || entry.topic[i] != ' ' )
                    entry.topic[++i] = *s;
            }

            entry.topic[++i] = '\0';
            entry.pos = pos + (long) n;
        }
    
        pos += n;
    }

    entry.len = (int) (pos - entry.pos);

    if ( fwrite(&entry, sizeof(help_indx), 1, wfp) < 1 )
    {
        fprintf(stderr, "error writing %s\n", indxfile);
        exit(1);
    }
  
    fclose(rfp);
    fclose(wfp);
  
    printf("   created '%s'\n", indxfile);
    printf("   %d topics indexed\n", ntopics);
    exit(0);
}
