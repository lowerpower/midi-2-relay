/*
 * Midi In
 *
 * Copyright (c) 2019 mycal 
 *
 */


#include "mytypes.h"
#include "config.h"
#include "debug.h"
#include "arch.h"
#include "net.h"
#include "midi.h"
#include "load_map.h"
#include "daemonize.h"
#include "yselect.h"
#if defined(WIN32)
#include "wingetopt.h"
#else
#include <termios.h> 
#endif

volatile int             go = 0;
MIDI                    *global_midi;
U8				        global_flag = 0;

//typedef unsigned char U8;
//typedef unsigned short U16;
//typedef unsigned int  U32;


//
// Unix handler if you need to handle CTRL-C or other signals
//
#if !defined(WIN32)
void
termination_handler(int signum)
{
    //if (global_midi->verbose) printf("term handler for signal %d\n", signum);
    printf("term handler for signal %d\n", signum);
    fflush(stdout);

    //sprintf(global_midi->last_msg, "term handler for signal %d\n", signum);
    //write_statistics(global_chat_ptr);
    go = 0;

    if ((SIGFPE == signum) || (SIGSEGV == signum) || (11 == signum))
    {
        yprintf("Midi Processor Terminated from Signal %d\n", signum);
        fflush(stdout);
        if (global_flag&GF_DAEMON) syslog(LOG_ERR, "Midi Processor Terminated from Signal 11\n");

#if defined(BACKTRACE_SYMBOLS)
        {
            // addr2line?                
            void* callstack[128];
            int i, frames = backtrace(callstack, 128);
            char** strs = backtrace_symbols(callstack, frames);
            yprintf("backtrace:\n");
            for (i = 0; i < frames; ++i)
            {
                yprintf("T->%s\n", strs[i]);
                if (global_flag&GF_DAEMON)  syslog(LOG_ERR, "T->%s\n", strs[i]);
            }
            free(strs);
            fflush(stdout);
        }
#endif
        exit(11);
    }

}

#else

BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
    switch (CEvent)
    {
    case CTRL_C_EVENT:
        yprintf("CTRL+C received!\n");
        break;
    case CTRL_BREAK_EVENT:
        yprintf("CTRL+BREAK received!\n");
        break;
    case CTRL_CLOSE_EVENT:
        yprintf("program is being closed received!\n");
        break;
    case CTRL_SHUTDOWN_EVENT:
        yprintf("machine is being shutdown!\n");
        break;
    case CTRL_LOGOFF_EVENT:
        return FALSE;
    }
    go = 0;

    return TRUE;
}

#endif


int
init_serial(MIDI *midi)
{
#if !defined(WIN32)
        struct termios options;
    //
    // Open Serial Port (note that this config is for raspberry pi 3 with serial port configured for midi)
    //
    midi->sfd = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);
    //midi->sfd = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY );
    if (midi->sfd == -1) {
        printf("Error no is : %d\n", errno);
        printf("Error description is : %s\n", strerror(errno));
        return(-1);
    }

    //raw mode. no echo, with 8n1 at 38400 baud 
    tcgetattr(midi->sfd, &options);
    cfsetspeed(&options, B38400);
    options.c_cflag &= ~CSTOPB;
    options.c_cflag |= CLOCAL;
    options.c_cflag |= CREAD;
    options.c_cc[VMIN]  = 1 ;//should_block ? 1 : 0;
    //options.c_cc[VMIN]  = 0 ;//should_block ? 1 : 0;
    options.c_cc[VTIME] = 0; 
    //options.c_cc[VTIME] = 2; 
    cfmakeraw(&options);
    tcsetattr(midi->sfd, TCSANOW, &options);
#endif
    
    return(0);
}


void
reset_relay(MIDI *midi)
{
    memset(midi->bitmask,0,BITMASK_SIZE);
    Send_UDP(midi, "set 0", strlen("set 0"));

}

int
process_midi_system(MIDI *midi, int command)
{
    int ret = 0;

    // Just look at high nibble for type
    switch (command)
    {
    case 0x80:  /*note off */
    default:
        break;
    }
    return(ret);
}

int
set_relay_map(MIDI *midi, int bit, int state)
{
    int index, mask;
    int ret = 1;

    // calculate relay bit position
    index = (bit-1) / 8;
    mask = 1 << ((bit-1) % 8);

    DEBUG1("setting index %d mask %d to %d (current %x) for bit %d\n", index, mask, state,midi->bitmask[index],bit);

    if (state)
    {
        // Set the bit
        midi->bitmask[index] |= mask;
    }
    else
    {
        // clear the bit
        midi->bitmask[index] &= ~mask;
    }

    return(ret);
}


int Bitmask_2_String(MIDI *midi)
{
    int     i,on=0;
    char    tstr[BUFFER_SIZE];


    midi->bit_string[0]=0;

    for(i=BITMASK_SIZE-1;i>=0;i--)
    {
        if(midi->bitmask[i])
        {
            on=1;
            sprintf(tstr,"%02x",midi->bitmask[i]);
            DEBUG1("bitmask %d = %s\n",i,tstr);
            strcat(midi->bit_string,tstr);
        }
        else if(on)
        {
            strcat(midi->bit_string,"00");
        }
    }
    
    if(0==strlen(midi->bit_string))
        strcat(midi->bit_string,"0");

    if(midi->verbose>2) printf("new bitstring %s\n",midi->bit_string);
    return(1);
}


int Send_UDP(MIDI *midi, char *buffer, int buffer_size)
{
    int ret;
    struct sockaddr_in  client;

    // udp send the set command
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = midi->target_ip.ip32;
    client.sin_port = htons((U16)(midi->target_port));


    if(midi->verbose>1) printf("sendto %s target port %d\n",buffer,midi->target_port);

    ret = sendto(midi->soc, buffer, buffer_size, 0, (struct sockaddr *)&client, sizeof(struct sockaddr));
	
	midi->send_timer=second_count();

    ysleep_usec(5);

    return(ret);

}


int Send_Bitmask_2_relay(MIDI *midi)
{
	int ret;
	char command_buffer[BUFFER_SIZE];

    Bitmask_2_String(midi);
	sprintf(command_buffer,"set %s",midi->bit_string);

    ret=Send_UDP(midi, command_buffer, strlen(command_buffer));

	return(ret);
}


int process_midi_note_on(MIDI *midi, int key, int channel, int velocity)
{
    set_relay_map(midi, midi->map[key][channel], 1);
    Send_Bitmask_2_relay(midi);
    return(1);
}

int process_midi_note_off(MIDI *midi,int key, int channel)
{
    set_relay_map(midi, midi->map[key][channel], 0);
    Send_Bitmask_2_relay(midi);
    return(1);
}


//
// Return 1 if we can process tyis type
//
int
support_midi_byte_type(MIDI *midi, char type)
{
    int ret = 0;

    // Just look at high nibble for type
    switch (type & 0xf0)
    {
    case 0x80:  /*note off */
        ret = 1;
        break;
    case 0x90:  /*note on */
        ret = 1;
        break;
    case 0xa0:  /*Polyphonic Pressure */
        break;
    case 0xb0:  /* Control Change */
        printf("*control change*\n");
        //reset_relay(midi);
        break;
    case 0xc0:  /* Program Change */
        break;
    case 0xd0:  /* Channel Pressure */
        break;
    case 0xe0:  /* Pitch Bend */
        printf("*pitch bend change*\n");
        //reset_relay(midi);
        break;
    case 0xf0:  /* System */
        ret = 1;
        break;
    default:
        printf("unsupported %x\n",(type & 0xf0));
        break;
    }
    return(ret);
}


int
process_midi_command(MIDI *midi)
{
    int ret = 0;
    int channel = 0;
    int key = 0;
    int velocity = 0;

    channel = (midi->status & 0xf);
    key = (midi->data1);
    velocity = (midi->data2);

    // process the midi command 
    switch (midi->status & 0xf0)
    {
    case 0x80:  /*note off */
        if(midi->verbose>1) printf("Turn Note %d on channel %d off velocity %d\n", key, channel, velocity);
        process_midi_note_off(midi,key,channel);
        ret = 1;
        break;
    case 0x90:  /*note on */
        if (0 == velocity)
        {
            if(midi->verbose>1) printf("Turn Note %d on channel %d off (note on velocity=0)\n", key, channel);
            process_midi_note_off(midi, key, channel);
        }
        else
        {
            if(midi->verbose>1) printf("Turn Note %d on channel %d on velocity %d\n", key, channel,velocity);
            process_midi_note_on(midi,key,channel,velocity);
        }
        break;
    case 0xf0:  /* System */
        process_midi_system(midi, midi->status & 0xf0);
        break;
    default:
        break;
    }
    return(ret);
}

//
//
//
void
process_midi_byte(MIDI *midi, char byte)
{
    //char type = 0;

    // is status byte?
    if (0x80 & byte)
    {
        // get type
        //
        // Midi Format
        //    | status  | data1    | data2     |
        //    |1tttnnnn | 0xxxxxxx | 0xxxxxxxx |
        //    t = type
        //    n = channel / subclass
        //
        // type = (byte & 0x7) >> 4;
        //
        midi->status = byte;
        midi->counter = 0;
        // 
        // do we support this type?
        //
        if (support_midi_byte_type(midi, midi->status))
        {
            if(0xf0==(midi->status & 0xf0))
            {
                // System bytes are only 1 byte
                process_midi_system(midi, midi->status & 0xf0);
            }
            else
            {
                // we process this type, set counter to 1
                DEBUG2("we support this set counter to 1\n");
                midi->counter = 1;
            }
        }
        else
        {
            //printf("Unsupported %x\n",(0xf0 & byte));
        }
        // done processing status byte
        return;
    }
    else
    {
        if (1 == midi->counter)
        {
            midi->data1 = byte;
            midi->counter = 2;
        }
        else
        {
            midi->data2=byte;
            process_midi_command(midi);
            midi->counter = 1;
        }
    }
}

//
// Actually do not do anything here
//
int process_udp_in(MIDI *midi)
{
    struct sockaddr_in  client;
    char                message[1025];
    int slen,ret=0;
    // just dump reply for now
    
    memset(&client,'\0',sizeof(struct sockaddr));
    slen=sizeof(struct sockaddr_in);
    ret=recvfrom(midi->soc, (char *)message, 1024, 0, (struct sockaddr *)&client, (socklen_t *) &slen);
    if(ret>0)
    {
        message[ret]=0;
        if(midi->verbose>1) printf("Incomming->%s",message);
    }

    return(ret);
}

//
// Banner for Software
//
void
startup_banner()
{
    //------------------------------------------------------------------
    // Print Banner
    //------------------------------------------------------------------
    printf("midi processor built " __DATE__ " at " __TIME__ "\n");
    printf("   Version " VERSION " -  (c)2019 mycal.net, Inc. All Rights Reserved\n");
    fflush(stdout);
}

//
// Usage for Software
//
void usage(int argc, char **argv)
{
    startup_banner();

    printf("usage: %s [-h] [-v(erbose)] [-d][pid file] [-f config_file]  \n", argv[0]);
    printf("\t -h this output.\n");
    printf("\t -v console debug output.\n");
    printf("\t -d runs the program as a daemon pid file must be specified.\n");
    printf("\t -f specify a config file.\n");
    printf("\t -t target IP:Port (default 127.0.0.1:5998).\n");
    exit(2);
}

int main(int argc, char **argv)
{
    int				c,sel;
    //int             file_ret = 0;
    U32				timestamp = second_count();
    MIDI            midi_static;
    MIDI            *midi=&midi_static;
    int             count = 0;
    int             read_count;
	char			tbuffer[BUFFER_SIZE];
    char            *subst;
	IPADDR			our_ip;
#if !defined(WIN32)
    char            buf2[8];
    //struct termios  options;
#endif


    // Set defaults
    memset(midi,0,sizeof(MIDI));
    strcpy(midi->map_file, "/var/www/html/uploads/map.txt");
    midi->target_port=1027;
    strcpy(midi->target_host,"127.0.0.1");
    midi->target_ip.ipb1=127;
    midi->target_ip.ipb2=0;
    midi->target_ip.ipb3=0;
    midi->target_ip.ipb4=1;
    //
    // Banner
    startup_banner();
    Yoics_Init_Select();



    //------------------------------------------------------------------
    // Initialize error handling and signals
    //------------------------------------------------------------------
    //	SetConsoleCtrlHandle(termination_handler,TRUE);
#if defined(WIN32) 
    if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE) == FALSE)
    {
        // unable to install handler... 
        // display message to the user
        yprintf("Error - Unable to install control handler!\n");
        exit(0);
    }
#else

    if (signal(SIGINT, termination_handler) == SIG_IGN)
        signal(SIGINT, SIG_IGN);
    if (signal(SIGTERM, termination_handler) == SIG_IGN)
        signal(SIGTERM, SIG_IGN);
    if (signal(SIGILL, termination_handler) == SIG_IGN)
        signal(SIGILL, SIG_IGN);
    if (signal(SIGFPE, termination_handler) == SIG_IGN)
        signal(SIGFPE, SIG_IGN);
    //if (signal (SIGSEGV , termination_handler) == SIG_IGN)
    //	signal (SIGSEGV , SIG_IGN);
#if defined(LINUX) || defined(MACOSX) || defined(IOS)
    if (signal(SIGXCPU, termination_handler) == SIG_IGN)
        signal(SIGXCPU, SIG_IGN);
    if (signal(SIGXFSZ, termination_handler) == SIG_IGN)
        signal(SIGXFSZ, SIG_IGN);
#endif
#endif
    //
    // Load config file first
    //
    while ((c = getopt(argc, argv, "f:u:l:d:vh")) != EOF)
    {
        switch (c)
        {
        case 0:
            break;
        case 't':
            break;
        case 'd':
            break;
        case 'v':
            midi->verbose++;
            break; 
        case 'f':
            // Do nothing, did it above
            break;
        case 'h':
            usage(argc, argv);
            break;
        default:
            usage(argc, argv);
            break;
        }
    }

    //
    // Load Command Line Args, they overrie config file, reset optarg=1 to rescan
    //
    optind = 1;
    while ((c = getopt(argc, argv, "f:u:l:d:vh")) != EOF)
    {
        switch (c)
        {
        case 0:
            break;
        case 'l':
            //log level
            midi->log_level = atoi(optarg);
            break;
        case 't':
            //target Port
            // Fix this to preserve command line
            // Specify serve to connect to <ip:port>
            // copy optarg to a temp buffer so we do not disterb the system argc
			memset(tbuffer, 0, BUFFER_SIZE);
			strncpy(tbuffer,optarg , BUFFER_SIZE-1);

            subst = strchr(tbuffer, ':');
            if (subst)
            	*subst = '\0';

            strncpy((char *)midi->target_host, tbuffer, MAX_PATH-1);
            midi->target_host[MAX_PATH - 1] = 0;
            if (subst)
            {
            	*subst = ':';

                if (++subst)
                {
                	midi->target_port = (U16)atoi(subst);
                }
            }
            break;
        case 'd':
            // Startup as daemon with pid file
            printf("Starting up as daemon\n");
            strncpy(midi->pidfile, optarg, MAX_PATH - 1);
            global_flag = global_flag | GF_DAEMON;
            break;
        case 'v':
            break;
        case 'f':
            // Do nothing, did it above
            break;
        case 'h':
            usage(argc, argv);
            break;
        default:
            usage(argc, argv);
            break;
        }
    }
    argc -= optind;
    argv += optind;


    if(midi->verbose) printf("verbose level at %d\n",midi->verbose);

    //
    // Load Map File
    //
    load_map(midi);

    if (0 > init_serial(midi))
    {
        printf("exiting, serial port failed to initialize.\n");
        return(1);
    }

    // Init UDP socket
	our_ip.ip32=0;
    midi->soc=udp_listener(0, our_ip);
    if(midi->soc<0)
    {
        printf("Failed to bind socket\n");
        exit(1);
    }
    printf("socket bound\n");
    set_sock_nonblock(midi->soc);
    Yoics_Set_Select_rx(midi->soc);


#if !defined(WIN32)
    //
    // Should Daemonize here if set
    //
    if (global_flag&GF_DAEMON)
    {
        if (midi->verbose) printf("Calling Daemonize\n");

        // Setup logging
        openlog("midi_processor", LOG_PID | LOG_CONS, LOG_USER);
        syslog(LOG_INFO, "Midi Processor built "__DATE__ " at " __TIME__ "\n");
        syslog(LOG_INFO, "   Version " VERSION " -  (c)2019 mycal.net All Rights Reserved\n");
        syslog(LOG_INFO, "Starting up as daemon\n");
        //syslog(LOG_INFO, "Bound to UDP %d.%d.%d.%d:%d on socket %d\n", sc.Bind_IP.ipb1, sc.Bind_IP.ipb2, sc.Bind_IP.ipb3, sc.Bind_IP.ipb4, sc.udp_listen_port, sc.udp_listen_soc);


        // Daemonize this
        daemonize(midi->pidfile, midi->run_as_user, 0, 0, 0, 0, 0);
    }
#endif


    // initialize send timer
    midi->send_timer=second_count();

    // Read Forever, or until killed
    go = 1;
    Yoics_Set_Select_rx(midi->sfd);
    while (go)
    {
        sel=0;
        // Read a byte from the serial port
        // read a byte from serial midi interface
        
        read_count=Yoics_Select(100);
        if(read_count>0)
        {
            if(Yoics_Is_Select(midi->sfd))
            {
                sel=1;
                count = read(midi->sfd, buf2, 1);
                if(count)
                {
                //    printf("count=%d\n",count);
                    process_midi_byte(midi, buf2[0]);
                }
                else
                {
                    if(midi->verbose>1)
                    {
                        printf(".");
                        fflush(stdout);
                    }
                }
                buf2[1] = 0;
                //printf("count= %d  --> %x\n", count, buf2[0]);
            }
            else
            {
                if(midi->verbose>1)
                {
                    printf(",");
                    ysleep_usec(1000); 
                    fflush(stdout);
                }
                ysleep_usec(10); 
            }
            // Check UDP read socke
            if(Yoics_Is_Select(midi->soc))
            {
                int udp_read=1;
                int max_read=3;
           
                if(midi->verbose>2)
                {
                    printf("is udp select\n");
                    fflush(stdout);
                }
                sel=1;
                while(udp_read && max_read--)
                {
                    udp_read=process_udp_in(midi);
                }
            }
            if(0==sel)
            {
                if(midi->verbose>1)
                {
                    printf("selected but no select handled\n");
                    fflush(stdout);
                }
            }
        }
        else
        {
            if(midi->verbose>2)
            {
                printf("readc=%d\n",read_count);
                fflush(stdout);
            }
        }

        // check background every so often
        if ((second_count() - timestamp) > 60)
        {
            DEBUG1("load map file check\n");
            load_map_if_new(midi);
            timestamp = second_count();
        }

        // Send Ping
        if((second_count()-midi->send_timer) > 65)
        {
            // send timer updated in send_udp
                if(midi->verbose>1) printf("Send Timer Expired, send ping.\n");
                Send_UDP(midi, "ping", strlen("ping"));
        }
        fflush(stdout);
    }
	

    printf("shutdown\n");
    fflush(stdout);

	return 0;
}



