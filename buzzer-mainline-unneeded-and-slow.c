/*
 * Compile: gcc -pthread -obuzzer-mainline-unneeded-and-slow buzzer-mainline-unneeded-and-slow.c -L/usr/local/lib
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_TIMINGS    85
#define DHT_PIN        5 // pin 5 = gpio71

int main( int argc, char *argv[] ) {
    //if ( wiringPiSetup() == -1 )
    //            exit(1);
    system("echo 71 > /sys/class/gpio/export");
    //pinMode(DHT_PIN, OUTPUT);
    system("echo 'out' > /sys/class/gpio/gpio71/direction");

        printf("Usage: %s [BeepsCount] [BeepLengthSeconds] [ToneMilliseconds] [PauseLenghtSeconds]\nDefault: %s 10 0.25 750 0.25\n", argv[0], argv[0]);
        int iNumBeeps = 10;
        if (argc >= 2) {
                iNumBeeps = strtol(argv[1], NULL, 10);
        }
        double dBeepLenghtInSeconds = 0.25;
        if (argc >= 3) {
                dBeepLenghtInSeconds = atof(argv[2]);
        }
        int iTone = 750;
        if (argc >= 4) {
                iTone = strtol(argv[3], NULL, 10);
        }
        double dPause = dBeepLenghtInSeconds;
        if (argc >= 5) {
                dPause = atof(argv[4]);
        }
        int iLenghtLimiterIndex = dBeepLenghtInSeconds * 1000000.0 / ( (double)(2 * iTone) );
        for (int j = 0; j < iNumBeeps; j++) {
                for (int i = 0; i < iLenghtLimiterIndex; i++) {
                        //digitalWrite(DHT_PIN, HIGH);
			system("echo '1' > /sys/class/gpio/gpio71/value");
                        usleep(iTone);
                        //digitalWrite(DHT_PIN, LOW);
			system("echo '0' > /sys/class/gpio/gpio71/value");
                        usleep(iTone);
                }
                if (j < iNumBeeps-1) usleep(dPause * 1000000);
        }
        //digitalWrite(DHT_PIN, LOW);
	system("echo '1' > /sys/class/gpio/gpio71/value");

        return 0;
}

