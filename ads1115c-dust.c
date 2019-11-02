// gcc -pthread -o ads1115c-dust ads1115c-dust.c  -L/usr/local/lib -lwiringPi
// asd1115.c read TMP37 temperature sensor ANC0
// operates in continuous mode
// pull up resistors in module caused problems
// used level translator - operated ADS1115 at 5V
// by Lewis Loflin lewis@bvu.net
// www.bristolwatch.com
// http://www.bristolwatch.com/rpi/ads1115.html

#include <wiringPi.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>    // read/write usleep
#include <stdlib.h>    // exit function
#include <stdint.h>
#include <inttypes.h>  // uint8_t, etc
#include <linux/i2c-dev.h> // I2C bus definitions
#include <time.h>

#define MAX_TIMINGS    85
#define DHT_PIN_DUST        5


int fd;
// Note PCF8591 defaults to 0x48!
int asd_address = 0x48;
int16_t val;
uint8_t writeBuf[3];
uint8_t readBuf[2];
float myfloat;

const float VPS = 4.096 / 32768.0; // volts per step

/*
The resolution of the ADC in single ended mode 
we have 15 bit rather than 16 bit resolution, 
the 16th bit being the sign of the differential 
reading.
*/

int main() {

    if ( wiringPiSetup() == -1 )
                exit(1);
    pinMode(DHT_PIN_DUST, OUTPUT);

  // open device on /dev/i2c-0
  if ((fd = open("/dev/i2c-0", O_RDWR)) < 0) {
    printf("Error: Couldn't open device! %d\n", fd);
    exit (1);
  }

  // connect to ADS1115 as i2c slave
  if (ioctl(fd, I2C_SLAVE, asd_address) < 0) {
    printf("Error: Couldn't find device on address!\n");
    exit (1);
  }

  // set config register and start conversion
  // AIN0 and GND, 4.096v, 128s/s
  // Refer to page 19 area of spec sheet
  writeBuf[0] = 1; // config register is 1
  writeBuf[1] = 0b11000010; // 0xC2 single shot off
  // bit 15 flag bit for single shot not used here
  // Bits 14-12 input selection:
  // 100 ANC0; 101 ANC1; 110 ANC2; 111 ANC3
  // Bits 11-9 Amp gain. Default to 010 here 001 P19
  // Bit 8 Operational mode of the ADS1115.
  // 0 : Continuous conversion mode
  // 1 : Power-down single-shot mode (default)

  writeBuf[2] = 0b11100101; // bits 7-0  0x85
  // Bits 7-5 data rate default to 100 for 128SPS
  // Bits 4-0  comparator functions see spec sheet.

  if (write(fd, writeBuf, 3) != 3) {// begin conversion
    perror("Write to register 1");
    exit (1);
  }
  readBuf[0] = 0;// set pointer to 0
  if (write(fd, readBuf, 1) != 1) {
    perror("Write register select");
    exit(-1);
  }
  
  // take 5 readings:

int16_t val_array[1000];
for (int i = 0; i < 1000; i++) { val_array[i] = 0; }

digitalWrite(DHT_PIN_DUST, LOW);
delayMicroseconds(280);

  int count = 1;
  while (1)   {
    if (read(fd, readBuf, 2) != 2) {// read conversion register
      perror("Read conversion");
      exit(-1);
    }
    val = readBuf[0] << 8 | readBuf[1]; // could also multiply by 256 then add readBuf[1]
val_array[count] = val;
    count++; // inc count
    if (count == 500)   break;

  } // end while loop



	delayMicroseconds(40);
	digitalWrite(DHT_PIN_DUST, HIGH);
	delayMicroseconds(9680);


  float fMax = 0;
  count = 1;
  while (1)   {
    val = val_array[count];
    if (val < 0)   val = 0;// with +- LSB sometimes generates very low neg number.
    myfloat = val * VPS; // convert to voltage
    //printf("Conversion number %d HEX 0x%02x DEC %d %4.3f volts. TS=%ld\n", count, val, val, myfloat);
if (myfloat > fMax) fMax = myfloat;
    count++; // inc count
    if (count == 500)   break;
  } // end while loop

  // power down ASD1115
  writeBuf[0] = 1;    // config register is 1
  writeBuf[1] = 0b11000011; // bit 15-8 0xC3 single shot on
  writeBuf[2] = 0b10000101; // bits 7-0  0x85
  if (write(fd, writeBuf, 3) != 3) {
    perror("Write to register 1");
    exit (1);
  }

  close(fd);

printf("Max = %4.2f V \n", fMax);
printf("DustDensity = %.2f ug/m3 \n", 100.0f * (fMax-0.12f) / 0.5f);


  return 0;
}
