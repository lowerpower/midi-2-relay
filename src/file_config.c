/*!																www.yoics.com			
 *---------------------------------------------------------------------------
 *! \file file_config.c
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
#include "mux.h"
#include "debug.h"

#define MAX_LINE_SIZE	1024

// config file strings
#define		MUX_TARGET_PORT  		"target_port"
#define		MUX_TARGET_HOST  		"target_host"
#define		MUX_VERBOSE 			"verbose"
#define		MUX_USER    			"run_as_user"
#define		MUX_SOCKET_LIST 		"socket_list"
#define		MUX_STATS_FILE			"stats_file"
#define		MUX_STATS_INTERVAL		"stats_interval"
#define		MUX_BIND_IP       		"bind_ip"


int
read_config(MUX *mux)
{
	char	*subst;
	char	*strt_p;
    int     ret=1;
	char	line[MAX_LINE_SIZE];
    FILE    *fp;

    // Read from file

    if(NULL == (fp = fopen( (char *) mux->config_file, "r")) )
	{
		yprintf("cannot open %s config file.\n",mux->config_file);
		ret=-1;
	}
	else
	{
        // File is open, read the config
		while(readln_from_a_file(fp, (char *) line, MAX_LINE_SIZE-4))
		{
			if(strlen((char *) line)==0)
				continue;

			subst=strtok_r((char *) line," \n",&strt_p);

			// Get Rid of whitespace
			while(*subst==' ')
				subst++;

			DEBUG1("readcmd->%s\n",subst);

			if(strlen( (char *) subst)==0)
			{
				// do nothing
            }
            else if (0 == strcmp((char *)subst, MUX_TARGET_PORT))
            {
                subst = strtok_r(NULL, " \n", &strt_p);
                mux->target_port = (U16)atoi((char *)subst);
                if(mux->verbose) printf("File load target port %d\n",mux->port);
            }
            else if (0 == strcmp((char *)subst, MUX_TARGET_HOST))
            {
                subst = strtok_r(NULL, " \n", &strt_p);
                if (strlen(subst))
                {
                    strncpy(mux->target_host, subst, MAX_PATH);
                    mux->target_host[MAX_PATH] = 0;
                    if(mux->verbose) printf("File load target host %s\n",mux->target_host);
                }
            }
            else if (0 == strcmp((char *)subst, MUX_VERBOSE))
            {
                subst = strtok_r(NULL, " \n", &strt_p);
                mux->verbose = (U16)atoi((char *)subst);
                if(mux->verbose) printf("File set verbose to %d\n",mux->verbose);
            }
			else if(0==strcmp((char *) subst, MUX_USER))
			{
				subst=strtok_r(NULL," \n",&strt_p);
                if (get_user(mux, subst) > 0)
                {
                    if (mux->verbose)  printf("File set drop privliges user, Damonize will attempt to drop privlages to user %s instead of nobody.\n", subst);
                }

			}
			else if(0==strcmp((char *) subst,MUX_BIND_IP))
			{
				subst=strtok_r(NULL,".\n",&strt_p);
                IP_Extract(&mux->bind_IP, subst);
                if(mux->verbose) printf("File set bind IP to %s\n",subst);
			}
            else if (0 == strcmp((char *)subst, MUX_SOCKET_LIST))
            {
                int argc = 0;

                subst = strtok_r(NULL, ".\n", &strt_p);
                if(mux->verbose) printf("File Load socket_list %s\n",subst);
                //argc = count_args(subst);
                parse_socket_list(mux, argc, &subst);
            }
        }
    }
    if(fp)
        fclose(fp);
    return(ret);
}

