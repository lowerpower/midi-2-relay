#ifndef __MIDI_H__
#define __MIDI_H__
//---------------------------------------------------------------------------
// midi.h							-
//---------------------------------------------------------------------------
// Version                                                                  -
//		0.1 Original Version April 3, 2019     							-
//																			-
// (c)2019 Mycal.net					-
//---------------------------------------------------------------------------




//
// GF flags, global flags
//
#define	GF_GO			0x01				/* go */
#define GF_DAEMON		0x02				/* we are a daemon */
#define GF_QUIET		0x08				/* no output */
#define GF_CMD_LINE		0x10				/*  */
#define GF_CMD_PROC		0x20				/* Turn on command line processor */
#define GF_BANG_STATUS	0x40				/* turn on stat output */

extern U8              global_flag;


// Custom File config for each product here, this one is for server channel config
typedef struct midi_config_
{
    // Target Relay Driver
    char        target_host[MAX_PATH+1];
    U16         target_port;
    SOCKET		soc;
    //
    int         verbose;
    int         log_level;
    int         auto_reload;

    // serial descripter file
    int         sfd;        

    // current relay mask (support 128 relays)
    char        bitmask[16];

    // relay map
    char        map[128][16];

    char        config_file[MAX_PATH];
    char        map_file[MAX_PATH];
    struct stat map_file_info;

    char        run_as_user[MAX_PATH];
    char        chroot[MAX_PATH];

    // Stats

    // Pidfile for daemon
    char        pidfile[MAX_PATH];
}MIDI;

#endif
