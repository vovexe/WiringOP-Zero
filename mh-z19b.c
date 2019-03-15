// gcc mh-z19b.c -omh-z19b
#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#define SERIAL_PORT_NAME "/dev/ttyS1"

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

int mhz19b_crc_verify(unsigned char * response) { // Function to verify MH-Z19 crc according to datasheet, valid message with crc: 0xFF 0x86 0x02 0xC3 0x40 0x00 0x45 0x00 0x30
	char crc = 0;
	for (int i = 1; i < 8; i++)
		crc += response[i];
	crc %= 256; // Truncate to 8 bit
	crc =~ crc & 0xFF; // Invert number with xor
	crc++;
	if (crc == response[8]) {
		printf("CRC OK\n");
		return 0;
	}
	else {
		printf("CRC Mismatch, calculated: 0x%02X\n", crc);
		return 1;
	}
}

int mhz19b_read()
{
    int fd, wlen;
    fd = open(SERIAL_PORT_NAME, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", SERIAL_PORT_NAME, strerror(errno));
        return -1;
    }
    serialport_set_interface_attribs(fd, B9600); // baudrate 9600, 8 bits, no parity, 1 stop bit
    serialport_set_mincount(fd, 0); // set to pure timed read, or comment this if wait for bytes forever
    
    //wlen = write(fd, "Hello!\n", 7);
    unsigned char send_bytes[] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79}; // Request reading CO2
    wlen = write(fd, send_bytes, 9);
    if (wlen != 9)
        printf("Error from write: %d, %d\n", wlen, errno);
    tcdrain(fd); // delay for output

    do { // simple noncanonical input
        unsigned char buf[10];
        int rdlen = read(fd, buf, sizeof(buf) - 1);
        if (rdlen > 0) {
#ifdef DISPLAY_STRING
            buf[rdlen] = 0;
            printf("Read %d: \"%s\"\n", rdlen, buf);
#else /* display hex */
            unsigned char *p;
            printf("Read %d:", rdlen);
            for (p = buf; rdlen-- > 0; p++)
                printf(" 0x%02X", *p);
            printf("\n");
#endif
			if (9 >= rdlen && 0xFF == buf[0] && 0x86 == buf[1] && 0 == mhz19b_crc_verify(buf) ) {
				int iCO2 = 256 * buf[2] + buf[3];
				int iTemperature = buf[4] - 40;
				printf("CO2: %d ppm, Temperature: %d *C\n", iCO2, iTemperature);
			}
        } else if (rdlen < 0) {
            printf("Error from read: %d: %s\n", rdlen, strerror(errno));
        } else {  /* rdlen == 0 */
            break;
            printf("Timeout from read\n");
        }               
        /* repeat read to get full message */
    } while (1);
	close(fd);
	return 0;
}

int main() {
    while (1) {
        mhz19b_read();
    }
    return 0;
}
