#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#include "tad.h" 

extern configuration Configuration;

// define adc chips addresses and channel modes
#define ADC_1 		0x6a //68
#define ADC_2 		0x69
#define ADC_CHANNEL1	0x9C
#define ADC_CHANNEL2	0xBC
#define ADC_CHANNEL3 	0xDC
#define ADC_CHANNEL4	0xFC

#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

const float varDivisior = 64; // from pdf sheet on adc addresses and config for 18 bit mode
static float varMultiplier = 0;

float getadc (int chn);  

void initVoltage() {
  // setup multiplier based on input voltage range and divisor value
  varMultiplier = (2.4705882/varDivisior)/1000;
	if (Configuration.batteryMultiplier==0) Configuration.batteryMultiplier=5.914996;
	if (Configuration.panelMultiplier==0) Configuration.panelMultiplier=5.813809;
	if (Configuration.SonarTempMultiplier==0) Configuration.SonarTempMultiplier=1.0;
	
	
}

void *readVoltagesThread(void *args)
{
	struct sensorGrid *readings = (struct sensorGrid *)args;
		
       		float battery,panel,SonarTemp;
		battery=0;panel=0;SonarTemp=0;
        while(1) {
		if (Configuration.batteryPin >0)
			battery = getadc(Configuration.batteryPin);
		if (Configuration.panelPin >0)
			panel = getadc(Configuration.panelPin);
		if(Configuration.SonarTempPin >0 ) 
			SonarTemp=getadc(Configuration.SonarTempPin);
      
		//printf ("batt=%f   panel=%f \n", battery,panel);
		if(battery > 5.5) battery = -1;
		if(panel > 5.5) panel = -1;
		readings->batteryVoltage = battery * Configuration.batteryMultiplier ; //5.914996 ;  //constant for  voltage partitor
		readings->panelVoltage   = panel   * Configuration.panelMultiplier     ;//5.813809 ;      //constant for voltage partitor

  		if(SonarTemp!=0.0) SonarTemp=1/SonarTemp;
		readings->SonarTemp=SonarTemp*Configuration.SonarTempMultiplier+Configuration.SonarTempAddConst;
		sleep(Configuration.voltageInterval);
	}
}

float getadc (int chn){
  unsigned int fh,dummy, adc, adc_channel;
  float val;
  __u8  res[4];
  // select chip and channel from args
  switch (chn){
    case 1: { adc=ADC_1; adc_channel=ADC_CHANNEL1; }; break;
    case 2: { adc=ADC_1; adc_channel=ADC_CHANNEL2; }; break;
    case 3: { adc=ADC_1; adc_channel=ADC_CHANNEL3; }; break;
    case 4: { adc=ADC_1; adc_channel=ADC_CHANNEL4; }; break;
    case 5: { adc=ADC_2; adc_channel=ADC_CHANNEL1; }; break;
    case 6: { adc=ADC_2; adc_channel=ADC_CHANNEL2; }; break;
    case 7: { adc=ADC_2; adc_channel=ADC_CHANNEL3; }; break;
    case 8: { adc=ADC_2; adc_channel=ADC_CHANNEL4; }; break;
    default: { adc=ADC_1; adc_channel=ADC_CHANNEL1; }; break;
  }
  // open /dev/i2c-0 for version 1 Raspberry Pi boards
  // open /dev/i2c-1 for version 2 Raspberry Pi boards
  fh = open("/dev/i2c-1", O_RDWR);
  ioctl(fh,I2C_SLAVE,adc);
  // send request for channel
  i2c_smbus_write_byte (fh, adc_channel);
  usleep (50000);
  // read 4 bytes of data
  i2c_smbus_read_i2c_block_data(fh,adc_channel,4,res);
  // loop to check new value is available and then return value
  while (res[3] & 128){
	  // read 4 bytes of data
	  i2c_smbus_read_i2c_block_data(fh,adc_channel,4,res);
  }
  usleep(50000);
  close (fh);

  // shift bits to product result
  dummy = ((res[0] & 0b00000001) << 16) | (res[1] << 8) | res[2];

  // check if positive or negative number and invert if needed
  if (res[0]>=128) dummy = ~(0x020000 - dummy);
 
  val = dummy * varMultiplier;
  return val;
}

