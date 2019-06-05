/*!																www.yoics.com			
 *---------------------------------------------------------------------------
 *! \file midi.c
 *  \brief Configuration file reader - 
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version June 3, 2006									-        
 *
 *---------------------------------------------------------------------------    
 *                                                             				-
 * Copyright (C) 2006, Yoics Inc, www.yoics.com								-
 *                                                                         	-
 * $Date: 2006/08/29 20:35:55 $
 *
 *---------------------------------------------------------------------------
 *
 * Notes:
 * 
 *
*/


#include "config.h"
#include "arch.h"
#include "midi.h"
#include "debug.h"

#define MAX_LINE_SIZE	1024

//
// Load the midi map file
//
int
load_map(MIDI *midi)
{
	char	*subst;
	char	*strt_p;
    int     ret=1;
	char	line[MAX_LINE_SIZE];
    FILE    *fp;
    int     note, channel, relay;

    // Read from file

    if(NULL == (fp = fopen( (char *) midi->config_file, "r")) )
	{
		yprintf("cannot open %s config file.\n",midi->config_file);
		ret=-1;
	}
	else
	{
        // File is open, read the config
        while (readln_from_a_file(fp, (char *)line, MAX_LINE_SIZE - 4))
        {
            if (strlen((char *)line) == 0)
                continue;
            // parse first item, should be "note channel relay" -or- "1 1 1"
            //
            // Get Note
            //
            subst = strtok_r((char *)line, " \n", &strt_p);
            //
            if (0 == subst)
                continue;

            // Get Rid of whitespace
            while (' ' == *subst)
                subst++;

            // Check for comment
            if ('#' == *subst)
                continue;

            if (strlen(subst) == 0)
                continue;

            note = atoi(subst);
            if ((note < 0) || (note > 127))
            {
                if (midi->verbose) printf("line is corrupt, ignore (note='%s')\n", subst);
                continue;
            }
            //
            // Get Channel
            //
            subst = strtok_r((char *)line, " \n", &strt_p);
            // Get Rid of whitespace
            while (' ' == *subst)
                subst++;
            if (strlen(subst) == 0)
            {
                if (midi->verbose) printf("line is corrupt, ignore (channel='%s')\n", subst);
                continue;
            }
            channel = atoi(subst);
            if ((channel < 0) || (channel > 16))
            {
                if (midi->verbose) printf("line is corrupt, ignore (channel='%s')\n", subst);
                continue;
            }
            //
            // Get Relay
            //
            subst = strtok_r((char *)line, " \n", &strt_p);
            // Get Rid of whitespace
            while (' ' == *subst)
                subst++;
            if (strlen(subst) == 0)
            {
                if (midi->verbose) printf("line is corrupt, ignore (relay='%s')\n", subst);
                continue;
            }
            relay = atoi(subst);
            if ((channel < 0) || (channel > 127))
            {
                if (midi->verbose) printf("line is corrupt, ignore (relay='%s')\n", subst);
                continue;
            }
            //
            // We get here we good, store
            //
            midi->map[note][channel] = relay;
        }
    }
    if(fp)
        fclose(fp);
    return(ret);
}

