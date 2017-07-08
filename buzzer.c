/*
 * Compile: gcc -pthread -obuzzer buzzer.c -L/usr/local/lib -lwiringPi
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_TIMINGS    85
#define DHT_PIN        5

int main( int argc, char *argv[] ) {
    if ( wiringPiSetup() == -1 )
		exit(1);
    pinMode(DHT_PIN, OUTPUT);
	
	int iTone = 750;
	int iNumBeeps = 10;
	double dBeepLenghtInSeconds = 0.25;
	int iLenghtLimiterIndex = dBeepLenghtInSeconds * 1000000.0 / ( (double)(2 * iTone) );
	for (int j = 0; j < iNumBeeps; j++) {
		for (int i = 0; i < iLenghtLimiterIndex; i++) {
			digitalWrite(DHT_PIN, HIGH);
			delayMicroseconds(iTone);
			digitalWrite(DHT_PIN, LOW);
			delayMicroseconds(iTone);
		}
		delayMicroseconds(dBeepLenghtInSeconds * 1000000);
	}
	digitalWrite(DHT_PIN, LOW);
	
	return 0;
}
