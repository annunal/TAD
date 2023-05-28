#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <wiringPi.h>
#include <pthread.h>
#include <termios.h>
#include <sys/time.h>
#include <time.h>
#include "tad.h"
/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))
 
/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))

#define CONFIGFILENAME "config.txt"
#define STATUSFILENAME "/var/www/status.txt"
#define LINE1 3

static char *commands[] = {
        "DEFMSG1","DEFMSG2","DEFMSG3","LINE1","LINE2","LINE3","PAGE","SIREN","SPEAKER","DELETE","RESET","PWR","TIMEZONE","NAME","RESTORE",
                        "TIMETO","IPADDR","SUBNET","GATEWAY","TIME","GSM","GSMRES", NULL
	};

static char *properties[] = {
        	"DEFMSG1","DEFMSG2","DEFMSG3","LINE1","LINE2","LINE3", "LINE4","MODE","REL", "SIREN", NULL
	};

char *status[MAX_PROP];

static FILE *otp, *itp;

configuration Configuration;
int currentTimeMs0, sec0=0;
int milli0;

FILE * openSerialPort(char * portName, int rate)
{ 
    struct termios serial;
    int fd = 0, ret;
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

    // Set up Serial Configuration
    serial.c_iflag = 0;
    serial.c_oflag = 0;
    serial.c_lflag = 0;
    serial.c_cflag = 0;

    serial.c_cc[VMIN] = 0;
    serial.c_cc[VTIME] = 0;

    serial.c_cflag = rate | CS8 | CREAD;

    tcsetattr(fd, TCSANOW, &serial); // Apply configuration

    return fdopen(fd, "a+");
}



void printStatus(FILE *fp)
{
	int idx;

	for(idx = 0; properties[idx]; idx++)
	    if(status[idx])
		fprintf(fp, "%s:%s\n", properties[idx], status[idx] );
}

void writeStatus()
{
	FILE *fs = fopen( STATUSFILENAME, "w+");
	if(fs) {
		printStatus(fs);
		fclose(fs);
	} else {
		perror("Unable to write the status file");
	}
}

void readStatus()
{
	char *line = NULL, *ptr, **propertyName;
	FILE *fp = NULL;
	int len, chars, prop;

	fp = fopen(STATUSFILENAME, "r");
	if(fp) {
		while ((chars = getline(&line, &len, fp)) != -1) {
			ptr=line;
			while(*ptr!=':') {
				*ptr=toupper(*ptr);
				ptr++;
			}
			for( prop=0, ptr = *(propertyName = properties); ptr; ptr=*(++propertyName), prop++)
				if(!strncmp(ptr, line, strlen(ptr)))
				{ // the property is present in the list and can be stored in status
					if(chars > 1 + strlen(ptr)) {
						char *end;
						ptr = line + 1 + strlen(ptr);	// skip property name and colon
						end = ptr + strlen(ptr);
						while( *--end == 13 || *end == 10) *end = '\0';
						status[prop] = strdup(ptr);
					}
					break;
				}
		}

		free(line);
		fclose(fp);
	    } else {
		perror("Unable to open the status file");
	    }
}

void parseLine(char * line)
{
        char *label, *value;

        if (!line || line[0] == '*' || line[0] == ' ' || strlen(line) == 1) return;

        label = ctrim(strtok(line,"=*"));
        value = strtok(NULL,"=*");

        if(value) value = ctrim(value);
        value = replace_str(value,"$EQ","=");
        value = replace_str(value,"$EQ","=");
        value = replace_str(value,"$EQ","=");
       //printf("parseLine:  %s = %s\n", label,value);
        if (strcasecmp(label,"SaveURL") == 0)
        {
                value=replace_str(value,"$IDdevice",Configuration.IDdevice);
                strcpy(Configuration.SaveURL, value);
		            return;
        }
        if (strcasecmp(label,"gpsURL") == 0)
        {
                value=replace_str(value,"$IDdevice",Configuration.IDdevice);
                strcpy(Configuration.gpsURL, value);
		            return;
        }
        if (strcasecmp(label,"AlertURL") == 0)
        {
                value=replace_str(value,"$IDdevice",Configuration.IDdevice);
                strcpy(Configuration.AlertURL, value);
		            return;
        }
        if (strcasecmp(label,"EmailURL") == 0)
        {
                value=replace_str(value,"$IDdevice",Configuration.IDdevice);
                strcpy(Configuration.EmailURL, value);
		            return;
        }
        if (strcasecmp(label,"EmailTo") == 0)
        {
                value=replace_str(value,"$IDdevice",Configuration.IDdevice);
                strcpy(Configuration.EmailTo, value);
		            return;
        }
        if (strcasecmp(label,"SMSURL") == 0)
        {
                strcpy(Configuration.SMSURL, value);
		            return;
        }
        if (strcasecmp(label,"SMSlist") == 0)
        {
                strcpy(Configuration.SMSlist, value);
		            return;
        }
        if (strcasecmp(label,"SMSuser") == 0)
        {
                strcpy(Configuration.SMSuser, value);
		            return;
        }
        if (strcasecmp(label,"SMSpwd") == 0)
        {
                strcpy(Configuration.SMSpwd , value);
		            return;
        }                
        if (strcasecmp(label,"PhotoCMD") == 0)
        {       value=replace_str(value,"$IDdevice",Configuration.IDdevice);
                strcpy(Configuration.PhotoCMD, value);
		            return;
        }
        if (strcasecmp(label,"PhotoTimeInterval") == 0)
        {       Configuration.PhotoTimeInterval = (float) atof(value);
		            return;
        }
        if (strcasecmp(label,"PhotoAlertLevel") == 0)
        {       Configuration.PhotoAlertLevel =  (int) atoi(value);
		            return;
        }

        if (strcasecmp(label,"IDdevice") == 0)
        {   if (strcasecmp(value,"$HOSTNAME") == 0)
			{
				char hostname[200];
				int er=gethostname(hostname,sizeof(hostname));
				strcpy(value,hostname);
			}
            strcpy(Configuration.IDdevice, value);
		        return;
        }

        if (!strcasecmp(label,"title")) {
		strcpy(Configuration.title, value);
		return;
	}
        if (!strcasecmp(label,"location")) {
		strcpy(Configuration.location, value);
		return;
	}
	 if (!strcasecmp(label,"position")) {
		strcpy(Configuration.position, value);
		return;
	}
        if (!strcasecmp(label,"watchFolder")) {
		strcpy(Configuration.watchFolder, value);
		return;
	}
        if (!strcasecmp(label,"watchFile")) {
		strcpy(Configuration.watchFile, value);
		return;
	}
        if (!strcasecmp(label,"n300")) {
		Configuration.n300 = atoi(value);
		return;
	}
        if (!strcasecmp(label,"n30")) {
		Configuration.n30 = atoi(value);
		return;
	}
        if (!strcasecmp(label,"Interval")) {
		Configuration.Interval = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"threshold")) {
		Configuration.threshold = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"ratioRMS")) {
		Configuration.ratioRMS = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"AddRMS")) {
		Configuration.AddRMS = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"backFactor")) {
		Configuration.backFactor = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"BaudRate")) {
		Configuration.sensorRate = atoi(value);
		return;
	}
        if (!strcasecmp(label,"Serial")) {
		strcpy(Configuration.sensorSerial, value);
		return;
	}
        if (!strcasecmp(label,"sensorMultFac")) {
                Configuration.sensorMultFac = (float) atof(value);
                return;
        }
        if (!strcasecmp(label,"sensorAddFac")) {
                Configuration.sensorAddFac = (float) atof(value);
                return;
        }
        if (!strcasecmp(label,"gpsSerial")) {
		strcpy(Configuration.gpsSerial, value);
		return;
	}
        if (!strcasecmp(label,"gpsRate")) {
		Configuration.gpsRate = atoi(value);
		return;
	}

		if (!strcasecmp(label,"gpsDeltaTSave")) {
		Configuration.gpsDeltaTSave = atoi(value);
		return;
		}
        if (!strcasecmp(label,"Commands")) {
		strcpy(Configuration.commandSerial, value);
		return;
	}
        if (!strcasecmp(label,"batteryPin")) {
		Configuration.batteryPin = atoi(value);
		return;
	}
        if (!strcasecmp(label,"batteryMultiplier")) {
		Configuration.batteryMultiplier = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"panelPin")) {
		Configuration.panelPin = atoi(value);
		return;
	}
        if (!strcasecmp(label,"panelMultiplier")) {
		Configuration.panelMultiplier = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"SonarTempPin")) {
		Configuration.SonarTempPin = atoi(value);
		return;
	}	     
	    if (!strcasecmp(label,"SonarMaxLevel")) {
		Configuration.SonarMaxLevel = atof(value);
		return;
	}	     
	    if (!strcasecmp(label,"SonarMinLevel")) {
		Configuration.SonarMinLevel = atof(value);
		return;
	}	     
	    if (!strcasecmp(label,"SonarMaxDifference")) {
		Configuration.SonarMaxDifference = atof(value);
		return;
	}	     
	
		if (!strcasecmp(label,"SonarTempMultiplier")) {
		Configuration.SonarTempMultiplier = (float) atof(value);
		return;
	}
	     if (!strcasecmp(label,"SonarTempAddConst")) {
		Configuration.SonarTempAddConst = (float) atof(value);
		return;
	}
        if (!strcasecmp(label,"voltageInterval")) {
		Configuration.voltageInterval = atoi(value);
		return;
	}
        if (!strcasecmp(label,"simSonar")) {
		Configuration.simSonar = atoi(value);
		return;
	}
        if (!strcasecmp(label,"SaveAllData")) {
		Configuration.SaveAllData = atoi(value);
		return;
	}
	
        if (!strcasecmp(label,"servo")) {
		strcpy(Configuration.servo, value);
		return;
	}
	
}

void readConfiguration(char * inputFile)
{
        FILE *infile;
        int lcount = 0;
        char line[400];

        Configuration.ratioRMS = 3;
        Configuration.threshold = 2.;
	Configuration.title[0] = '\0';
	Configuration.location[0] = '\0';
	Configuration.SaveURL[0] = '\0';
	Configuration.watchFolder[0] = '\0';
	Configuration.watchFile[0] = '\0';
	Configuration.sensorSerial[0] = '\0';
	Configuration.gpsSerial[0] = '\0';
	Configuration.commandSerial[0] = '\0';

        printf("Reading file: %s\n",inputFile);
        /* Open the file.  If NULL is returned there was an error */
        if(!(infile = fopen(inputFile, "r"))) {
                printf("Error Opening File.\n");
                return;
        }

        while( fgets(line, sizeof(line), infile) ) {
                /* Get each line from the infile */
                lcount++;
                /* print the line number and data */
                printf("Line %d: %s", lcount, line);
                parseLine(line);
        }

        fclose(infile);  /* Close the file */
        printf("\n\nEnd of Reading file: %s\n",inputFile);
}

int main(int argc, char *argv[]) 
{
  struct sensorGrid readings;
  int fd, wd, idx;
  char fullname[FILENAME_MAX], tmpname[FILENAME_MAX], logCommands[4096];
  char hostname[200];
  int er=gethostname(hostname,sizeof(hostname));
  
  
  printf("Hostname=%s\n",hostname);
  readConfiguration(CONFIGFILENAME);
  
  if (argc>=2) 
  {
  	if (strcasecmp(argv[1],"retry")==0)
  	{   char dateName[32], currDate[32];
  		if(argc==3) strcpy(dateName,argv[2]);
  		else {
  			
  			time_t now = (time_t) time(0);
  			struct tm *gmtm = (struct tm *) gmtime(&now);
  			strftime(dateName, sizeof(dateName), "retry_%Y-%m-%d.txt", gmtm);
  			}
  		retryToTransmit(dateName);
  		return;
  	}
  }
  //if(argc != 3 && argc != 4) {
    //fprintf(stderr, "Usage: %s foldername filename [output]\n", argv[0]);
    //return 1;
  //}

  //if(argc == 4) {
  if(Configuration.commandSerial[0]) {
    otp = openSerialPort(Configuration.commandSerial, B9600);
    if(otp) {
      itp = otp;
    } else {
      perror( "Unable to append to output device" );
      otp = stdout;
      itp = stdin;
    }
  } else {
    otp = stdout;
    itp = stdin;
  }

  /* read configuration */
  for(idx = 0; idx < MAX_PROP; idx++) status[idx] = NULL;

  readStatus(status);

  /* set up communication with the siren */
  
	  if (wiringPiSetupGpio () == -1)  
	  {  
		 perror( "Can't initialise wiringPi" );
		 return -2;
	  }  
	readings.pressure = 0;
  readings.temperature = 0;
  readings.distance = 0;
  initSensors();  
  
  sleep(1);  // sleep 1 s to have at least 1 sensor value
  initAnalysis();  
  
  // lcd = lcdInit (2, 16, 4, RS, STRB, 0, 1, 2, 3, 4, 5, 6, 7);

  startSensorsReading(&readings);

  /* loop on file change events and execute commands */
  while( 1 ) {
    sleep(1);
  }
  return 0;
}

void printLog(char *line)
{
	if(otp) {
		fprintf(otp, line);
		fflush(otp);
	}
	char dateName[32];
	time_t now = (time_t) time(0);
	struct tm *gmtm = (struct tm *) gmtime(&now);
        strftime(dateName, sizeof(dateName), "%Y-%m-%d.log", gmtm);

	FILE *logFile = fopen(dateName, "a+");
	if(logFile) {
		fprintf(logFile, line);
		fclose(logFile);
	}
}

void saveAllData(struct sensorGrid *rowdata, int action)
{
  char dateName[32], dateName1[32], currDate[32];
  time_t now = (time_t) time(0);
	struct tm *gmtm = (struct tm *) gmtime(&now);
 
	if(action==0)
  {
      strftime(dateName, sizeof(dateName), "/tmp/TAD/AllData_%Y-%m-%d.log", gmtm);
      strftime(currDate, sizeof(currDate), "%d\/%m\/%Y", gmtm);
      
      //char buffer[26];
/*      int millisec;
      struct tm* tm_info;
      struct timeval tv;
    
      gettimeofday(&tv, NULL);
    
      millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
      if (millisec>=1000) { // Allow for rounding up to nearest second
        millisec -=1000;
        tv.tv_sec++;
      }
    
      tm_info = localtime(&tv.tv_sec);
    
      strftime(buffer, 26, "%d\/%m\/%Y %H:%M:%S", tm_info);
      sprintf(currDate,"%s.%03d", buffer, millisec);
*/
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    //struct timespec  tp;
    //clock_gettime(CLOCK_REALTIME, &tp);
    //int milli = tp.tv_nsec; //curTime.tv_usec / 1000;
    
    char buffer [80];
    strftime(buffer, 80, "%d\/%m\/%Y %H:%M:%S", localtime(&curTime.tv_sec));
    
    if (curTime.tv_sec != sec0 ) {
      sec0=curTime.tv_sec;
      //currentTimeMs0=millis();
      milli0=-125; 
      }
      
    int milli=milli0+125;
    if(milli>999) milli=999;
    milli0=milli;
       
    //char currentTime[84] = "";
    sprintf(currDate, "%s.%03d", buffer, milli);
    //printf("current time: %s \n", currentTime);

/*    struct timeval tp;
    gettimeofday(&tp, 0);
    time_t curtime = tp.tv_sec;
    struct tm *t = localtime(&curtime);
    sprintf(buffer,"%s %02d:%02d:%02d:%03d",currDate, t->tm_hour, t->tm_min, t->tm_sec, tp.tv_usec/1000);
 
*/
      //printf("CurrDate = %s   buffer=%s\n",currDate,buffer);
	    FILE *logFile = fopen(dateName, "a+");
	    if(logFile) 
      {
         fprintf(logFile,"%s %f\n", currDate, rowdata->distance);
         fclose(logFile);
       }
   }
   else
   {
//        strftime(dateName, sizeof(dateName), "/tmp/AllData_%Y-%m-%d.log", gmtm);
//        strftime(dateName1, sizeof(dateName1), "AllData_%Y-%m-%d.log", gmtm);
        char cmd[4096];
//        sprintf(cmd, "cat %s >> %s", dateName, dateName1);
//        system(cmd);
        //printf("rm  %s",dateName);
        sprintf(cmd, "rm -f /tmp/Enter* ", dateName);
        system(cmd);

   }
 	
}
void printRetry(char *line)
{
	char dateName[32], currDate[32];
	time_t now = (time_t) time(0);
	struct tm *gmtm = (struct tm *) gmtime(&now);
  strftime(dateName, sizeof(dateName), "retry_%Y-%m-%d.txt", gmtm);
	strftime(currDate, sizeof(currDate), "%d\/%m\/%Y %H:%M:%S", gmtm);

	FILE *logFile = fopen(dateName, "a+");
	fprintf(logFile,"echo \"%s\"\n",currDate);
	fprintf(logFile,"%s\n",line);
	fclose(logFile);
}

void retryToTransmit(char * fname)
{
        FILE * infile;
        char line[1000];
		char dummy[1000];
		if(!fileExists(fname)) return;

		system("rm -f retryStorex.txt");
        if((infile = fopen(fname, "r")) == NULL) {
                printf("File %s does not exists\n", fname);
                return;
                }

        int isConnected=TRUE;
        FILE * retryStore1=fopen("retryStorex.txt", "w");
        while( fgets(line, sizeof(line), infile) != NULL ) {
                //printf("line x: %s\n",line);
			if( !strstr(line, "echo") && line[0] !=10	)
			{
				if (isConnected ) 
				{ // && strcasecmp(line[0],"e") != 0) { 
					printf("RETR: %s\n",line);
					system(line);
					if ( fileContains("outlogwget.txt","EnterData.aspx") || fileContains("outlogwget.txt","EnterAlert.aspx") )
							{ 
							system("rm -f EnterData.aspx*");
							printf("Transmitted\n");
							//LastTimeConnected_s = CurrentTime_s;

							}
					else if ( fileContains("outlogwget.txt","EnterAlert.aspx") )
							{ 
							system("rm -f EnterAlert.aspx*");
							printf("Transmitted\n");
							//LastTimeConnected_s = CurrentTime_s;
							}
					else
                        { //isConnected=FALSE ;
						  printf("Not transmitted, stored\n");
                          fprintf(retryStore1, "%s\n",line);
						  fflush(retryStore1);
                        }
				}
				else
				     fprintf(retryStore1, "%s\n",line);
					 fflush(retryStore1);
			}
        } 
        fclose(retryStore1);
		fclose(infile);
        if (fileExists("retryStorex.txt"))
			sprintf(dummy,"sudo mv -f retryStorex.txt %s",fname);
			printf("%s\n",dummy);
			system(dummy);
		
        
}
