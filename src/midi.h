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

#define BITMASK_SIZE       32               // support 256 relays

#define BUFFER_SIZE     2048



// Custom File config for each product here, this one is for server channel config
typedef struct midi_config_
{
    // Target Relay Driver
    char        target_host[MAX_PATH+1];
    IPADDR      target_ip;
    U16         target_port;
    SOCKET		soc;
    U32         send_timer;
    //
    int         verbose;
    int         log_level;
    int         auto_reload;

    // serial descripter file
    int         sfd;        
    char        serial_device[MAX_PATH];

    // current relay mask (support 256 relays)
    char        bitmask[BITMASK_SIZE];
    char        bit_string[BUFFER_SIZE];

    // relay map
    char        map[128][16];

    char        config_file[MAX_PATH];
    char        map_file[MAX_PATH];
    struct stat map_file_info;

    char        run_as_user[MAX_PATH];
    char        chroot[MAX_PATH];

    // Current midi state
    int         counter;
    char        status;
    char        type;
    char        data1;
    char        data2;

    // Stats

    // Pidfile for daemon
    char        pidfile[MAX_PATH];
}MIDI;

#endif
