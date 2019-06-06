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
#include "midi.h"
#include "load_map.h"
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
    if (global_midi->verbose) printf("term handler for signal %d\n", signum);

    //sprintf(global_midi->last_msg, "term handler for signal %d\n", signum);
    //write_statistics(global_chat_ptr);
    go = 0;

    if ((SIGFPE == signum) || (SIGSEGV == signum) || (11 == signum))
    {
        yprintf("Midi Processor Terminated from Signal %d\n", signum);
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


init_serial(MIDI *midi)
{
#if !defined(WIN32)
    //
    // Open Serial Port (note that this config is for raspberry pi 3 with serial port configured for midi)
    //
    midi->sfd = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY);
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
    cfmakeraw(&options);
    tcsetattr(midi->sfd, TCSANOW, &options);
#endif
    
    return(0);
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
    int				c;
    int             file_ret = 0;
    U32				timestamp = second_count();
    MIDI            midi_static;
    MIDI            *midi=&midi_static;
    int             count = 0;
#if !defined(WIN32)
    struct termios  options;
#endif


    // Set defaults
    memset(midi,0,sizeof(MIDI));
    strcpy(midi->map_file, "map.txt");

    //
    // Banner
    startup_banner();



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
           //midi->target_port = atoi(optarg);
           // midi->target_host = atoi(optarg);
            break;
        case 'd':
            // Startup as daemon with pid file
            printf("Starting up as daemon\n");
            strncpy(midi->pidfile, optarg, MAX_PATH - 1);
            global_flag = global_flag | GF_DAEMON;
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
    argc -= optind;
    argv += optind;

    //
    // Load Map File
    //
    load_map(midi);

    if (0 > init_serial(midi))
    {
        printf("exiting, serial port failed to initialize.\n");
        return(1);
    }

  

#if !defined(WIN32)
    //
    // Should Daemonize here if set
    //
    if (global_flag&GF_DAEMON)
    {
        if (sc.verbose) printf("Calling Daemonize\n");

        // Setup logging
        openlog("midi_processor", LOG_PID | LOG_CONS, LOG_USER);
        syslog(LOG_INFO, "Midi Processor built "__DATE__ " at " __TIME__ "\n");
        syslog(LOG_INFO, "   Version " VERSION " -  (c)2019 mycal.net All Rights Reserved\n");
        syslog(LOG_INFO, "Starting up as daemon\n");
        syslog(LOG_INFO, "Bound to UDP %d.%d.%d.%d:%d on socket %d\n", sc.Bind_IP.ipb1, sc.Bind_IP.ipb2, sc.Bind_IP.ipb3, sc.Bind_IP.ipb4, sc.udp_listen_port, sc.udp_listen_soc);


        // Daemonize this
        daemonize(sc.pidfile, sc.run_as_user, 0, 0, 0, 0, 0);
    }
#endif



    // Read Forever, or until killed
    go = 1;
    while (go)
    {
        // Read a byte from the serial port
#if !defined(WIN32)
        count = read(midi->sfd, buf2, 1);
        buf2[1] = 0;
        printf("count= %d  --> %x\n", count, buf2[0]);
#endif

        // check background every so often
        //load_map_if_new(midi);
    }
	

printf("shutdown\n");

	return 0;
}



