#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include "tad.h"

static FILE *ttyFP = NULL;
#define BUFFERSIZE (1024)

//#define MAXLEVEL   10.0
//#define MINLEVEL   0.5

static char reading[BUFFERSIZE];
static int initialized = 0;

extern configuration Configuration;

FILE * openSerialPort1(char * portName, int rate)
{
    struct termios serial;
    int fd = 0, ret, speed;
    printf("Opening serial port %s\n", portName);

    fd = open(portName, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1) {
        perror(portName);
        return NULL;
    }

    ret = fcntl(fd, F_SETFL, O_RDWR );
    if (tcgetattr(fd, &serial) < 0) {
        perror("Getting configuration");
        return NULL;
    }

    switch(rate) {
	case 9600: speed = B9600;
		break;
	case 57600: speed = B57600;
		break;
    }
    cfsetispeed(&serial, speed);                 /* Set the baud rates to 9600 */
    cfsetospeed(&serial, speed);

    /* Enable the receiver and set local mode */
         serial.c_cflag |= (CLOCAL | CREAD);
         serial.c_cflag &= ~PARENB; /* Mask the character size to 8 bits, no parity */
         serial.c_cflag &= ~CSTOPB;
         serial.c_cflag &= ~CSIZE;
         serial.c_cflag |=  CS8;                              /* Select 8 data bits */
         serial.c_cflag &= ~CRTSCTS;               /* Disable hardware flow control */

                                                                         /* Enable data to be proc essed as raw input */
         serial.c_lflag &= ~(ICANON | ECHO | ISIG);


    tcsetattr(fd, TCSANOW, &serial); // Apply configuration

    return fdopen(fd, "a+");
}

void initSonar()
{
	ttyFP = openSerialPort1(Configuration.sensorSerial, Configuration.sensorRate);
	initialized = (ttyFP != NULL);
}

void *readDistanceThread(void *args)
{
	struct sensorGrid *readings = (struct sensorGrid *)args;
  struct sensorGrid *rowData = (struct sensorGrid *)args;
	float measure, average, measure0,average0;
	int nSamples;
	char *s, *p, *end = reading + BUFFERSIZE;
	int index,index0;
	float value;
  if(!initialized) return;

	average0=-1;
	if (Configuration.simSonar==1) {
	// to have printout w/o sensor
		while (1)
		{
			readings->distance=3.24;
			addMeasure(readings,-1);
			sleep(1);
		}
	}
	while(1) {
		for(p=reading;p<end;p++) *p=0;
		p = read(ttyFP->_fileno, reading, BUFFERSIZE);
		p=reading; 
		//while(!*p) p++;
		//s=p;
		//for(;p < end;p++) if(*p=='\r') *p=' ';
		//*--p=0;
		//p=s;
		//printf("%s\n", p);
		average = 0.0;
		nSamples = 0;
    measure0=-1;
		while(p < end)
		{
			while(*p++ != 'R' && p < end);
			while(*p == '0' && p < end) p++;
			if(p < end) {
			//printf("%s\n", p);
				measure = atof(p)/1000;
				if (Configuration.SaveAllData==1) {
					rowData->distance=measure;
					saveAllData(rowData,0);
				}
				if(measure>Configuration.SonarMinLevel && measure<Configuration.SonarMaxLevel & (fabs(measure-measure0)<Configuration.SonarMaxDifference || measure0==-1))
				{
				average +=measure;
				measure0=measure;
				nSamples +=1;
				}
				//else
					//printf("excluded measure: %f\n",measure);
//				average = average + (measure - average) / ++nSamples;
			}
		}
		 if(nSamples>1) 
		{
			average /=nSamples; 
		        if(average0==-1)average0=average;
			if(fabs(average-average0)<Configuration.SonarMaxDifference*5) {	
			readings->distance = average;
			//printf("average= %f\n",average); 
			addMeasure(readings,-1);
			average0=average;
	 		}
			//else
			  //printf("Average skipped, too large from previous: %f",average);	
		}
		sleep(1);
	}
	return NULL;
}

