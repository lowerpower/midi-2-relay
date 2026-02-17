/*!																www.mycal.net			
 *---------------------------------------------------------------------------
 *! \file load_map.c
 *  \brief load the midi map fiel 
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version April 15, 2019									-        
 *
 *---------------------------------------------------------------------------    
 *                                                             				-
 * Copyright (C) 2019, www.mycal.net         								-
 *                                                                         	-
 *
 *---------------------------------------------------------------------------
 *
 * Notes:
 * 
 *
*/


#include "config.h"
#include "mytypes.h"
#include "midi.h"
#include "arch.h"
#include "debug.h"
#include "load_map.h"

#define MAX_LINE_SIZE	1024

//
// load the map only if file has changed based by date last loaded
//
int
load_map_if_new(MIDI *midi)
{
    time_t old_time = midi->map_file_info.st_mtime;

    if (-1 != stat(midi->map_file, &midi->map_file_info))
    {
        // Must use difftime to be completly portable
        if ((difftime(old_time, midi->map_file_info.st_mtime)) || (0 == old_time))
        {
            // File has changed, reload
            //if (midi->verbose > 0) ytprintf("Reload Map File %s\n",midi->map_file);
            return(load_map(midi));
        }
    }
    else
    {
        if (midi->verbose > 1) ytprintf("Failed stat map file %s\n", midi->map_file);
    }
    return(0);
}



//
// load a map into the midi structure, if file is valid and open, the map is destroyed 
//
int
load_map(MIDI *midi)
{
	char	*subst;
	char	*strt_p;
    int     ret=1;
	char	line[MAX_LINE_SIZE];
	char    *line_ptr=line;
    FILE    *fp;
    int     note;
    int     channel;
    int     relay;

    // Read from file

    if(NULL == (fp = fopen( (char *) midi->map_file, "r")) )
	{
		yprintf("cannot open %s map file.\n",midi->map_file);
		ret=-1;
	}
	else
	{
        if (midi->verbose > 0) ytprintf("Reload Map File %s\n",midi->map_file);
	    // map file is open, clear out the midi map
	    memset(midi->map,0,sizeof(midi->map));
        // File is open, read the config
		while(readln_from_a_file(fp, (char*) line, MAX_LINE_SIZE-4))
		{
		    line_ptr=line;

			if(strlen(line_ptr)==0)
				continue;

			DEBUG2("read->%s\n",line_ptr);

            // Get Rid of whitespace
            while(' '==*line_ptr)
                line_ptr++;

            // check for commment, if so ignore
            if('#'==*line_ptr)
                continue;


            // Get Note
			subst=strtok_r((char *) line_ptr," \n",&strt_p);

            // if we have a note, else continue
            if(0==subst)
                continue;

            // get actual note
            note=atoi(subst);
            if((note>127) || (note<0))
               continue; 

            // get channel
            subst = strtok_r(NULL, " \n", &strt_p);
            // if we have a channel, else continue
            if(0==subst)
                continue;
            // get actual channel 
            channel=atoi(subst);
            if((channel<0) || (channel>15))
                continue;

            // get relay 
            subst = strtok_r(NULL, " \n", &strt_p);
            // if we have a channel, else continue
            if(0==subst)
                continue;
            // get actual channel 
            relay=atoi(subst);
            if((relay>255) || (relay<1))
                continue;

			// Got valid configuration write it to map  
			if(midi->verbose>1)
			    printf("Loading Note %d, channel %d to relay %d\n",note,channel,relay);
			
			// load it
			midi->map[note][channel]=relay;

            //load the currently loaded file with info
            stat(midi->map_file, &midi->map_file_info);
            fflush(stdout);
        }
    }
    if(fp)
        fclose(fp);
    return(ret);
}
