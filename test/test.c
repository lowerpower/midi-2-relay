#include <stdio.h>
#include <unistd.h> 
#include <fcntl.h> 
#include <termios.h> 
#include <errno.h>

int
main()
{
    char buf2[10];
    int count=0;
    struct termios options;



// Open the hardware UART, this is pre setup for midi in config
int sfd = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY ); 
if (sfd == -1){
    printf ("Error no is : %d\n", errno);
    printf("Error description is : %s\n",strerror(errno));
    return(-1);
}

printf("sfd=%d\n",sfd);

//cfsetospeed(&options,speed); //for output speed    
//cfsetispeed(&options,speed);  //for input speed


// set for 38400 for midi
//raw mode. no echo, with 8n1 at 38400 baud 
tcgetattr(sfd, &options);
cfsetspeed(&options,B38400);
options.c_cflag &= ~CSTOPB;
options.c_cflag |= CLOCAL;
options.c_cflag |= CREAD;
cfmakeraw(&options);
tcsetattr(sfd, TCSANOW, &options);


while(1)
{
    count=read(sfd,buf2,1);
    buf2[1]=0;
    printf("count= %d  --> %x\n",count,buf2[0]);
}


}



