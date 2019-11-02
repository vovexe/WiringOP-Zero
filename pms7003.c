// gcc pms7003.c -opms7003
#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#define SERIAL_PORT_NAME_PLANTOWER "/dev/ttyS2"
typedef struct {
  int frameLength;
  int standartPM10, standartPM25, standartPM100;
  int environmentPM10, environmentPM25, environmentPM100;
  int particles03um, particles05um, particles10um, particles25um, particles50um, particles100um;
  int unused;
  int checkCode;
} plantowerData_t;

int serialport_set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void serialport_set_mincount(int fd, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5; // half second timer

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}

int getPlantowerPMSMetrics(plantowerData_t* _ptrPlantowerData) {
    int fd, iResult = 0;
    fd = open(SERIAL_PORT_NAME_PLANTOWER, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", SERIAL_PORT_NAME_PLANTOWER, strerror(errno));
        return -1;
    }
    serialport_set_interface_attribs(fd, B9600); // baudrate 9600, 8 bits, no parity, 1 stop bit
    serialport_set_mincount(fd, 0); // set to pure timed read, or comment this if wait for bytes forever

	unsigned long readStartTime = time(NULL);
	while (time(NULL) - readStartTime < 5) {
		unsigned char buf[30]; // buffer for input data, format described here: https://download.kamami.pl/p566824-pmsa003f.pdf
		memset(buf, 0, 30);
		int rdlen = read(fd, buf, 1);
		if (rdlen < 1) continue;
		if (0x42 == buf[0]) { // first byte of signature found
			int rdlen = read(fd, buf, 1);
			if (rdlen < 1) continue;
			if (0x4D == buf[0]) { // second byte of signature found
				int rdlen = read(fd, buf, 30);
				if (rdlen < 30) continue;
				uint16_t crc = 0x42 + 0x4D;
				for (int i = 0; i < 28; i++)
					crc += buf[i];
				_ptrPlantowerData->frameLength = buf[0] * 256 + buf[1];
				_ptrPlantowerData->standartPM10 = buf[2] * 256 + buf[3];
				_ptrPlantowerData->standartPM25 = buf[4] * 256 + buf[5];
				_ptrPlantowerData->standartPM100 = buf[6] * 256 + buf[7];
				_ptrPlantowerData->environmentPM10 = buf[8] * 256 + buf[9];
				_ptrPlantowerData->environmentPM25 = buf[10] * 256 + buf[11];
				_ptrPlantowerData->environmentPM100 = buf[12] * 256 + buf[13];
				_ptrPlantowerData->particles03um = buf[14] * 256 + buf[15];
				_ptrPlantowerData->particles05um = buf[16] * 256 + buf[17];
				_ptrPlantowerData->particles10um = buf[18] * 256 + buf[19];
				_ptrPlantowerData->particles25um = buf[20] * 256 + buf[21];
				_ptrPlantowerData->particles50um = buf[22] * 256 + buf[23];
				_ptrPlantowerData->particles100um = buf[24] * 256 + buf[25];
				_ptrPlantowerData->unused = buf[26] * 256 + buf[27];
				_ptrPlantowerData->checkCode = buf[28] * 256 + buf[29];
				if (crc != _ptrPlantowerData->checkCode) continue;
				//printf("crc ok: %d\n", crc);
				iResult = 1;
				break;
			}
		}
	}
	close(fd);
	return iResult;
}

int main() {
    //while (1) {
	plantowerData_t plantowerData;
    if (getPlantowerPMSMetrics(&plantowerData)) {
      printf("---------------------------------------\nConcentration Units (standard)\n");
      printf("\t\tPM 1.0: %d\n", plantowerData.standartPM10);
      printf("\t\tPM 2.5: %d\n", plantowerData.standartPM25);
      printf("\t\tPM 10: %d\n", plantowerData.standartPM100);
      printf("---------------------------------------\nConcentration Units (environmental)\n");
      printf("\t\tPM 1.0: %d\n", plantowerData.environmentPM10);
      printf("\t\tPM 2.5: %d\n", plantowerData.environmentPM25);
      printf("\t\tPM 10: %d\n", plantowerData.environmentPM100);
      printf("---------------------------------------\n");
      printf("Particles > 0.3um / 0.1L air:%d\n", plantowerData.particles03um);
      printf("Particles > 0.5um / 0.1L air:%d\n", plantowerData.particles05um);
      printf("Particles > 1.0um / 0.1L air:%d\n", plantowerData.particles10um);
      printf("Particles > 2.5um / 0.1L air:%d\n", plantowerData.particles25um);
      printf("Particles > 5.0um / 0.1L air:%d\n", plantowerData.particles50um);
      printf("Particles > 10.0 um / 0.1L air:%d\n", plantowerData.particles100um);
      printf("---------------------------------------\n");
    }

    //}
    return 0;
}