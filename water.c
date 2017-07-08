/*
 *  dht.c:
 *    read temperature and humidity from DHT11 or DHT22 sensor
 * Compile: gcc -pthread -owater water.c -L/usr/local/lib -lwiringPi
 */
  
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
  
#define MAX_TIMINGS    85
#define DHT_PIN        1

int main( int argc, char *argv[] )
{
    //printf( "Raspberry Pi DHT11/DHT22 temperature/humidity test\n" );
  
    if ( wiringPiSetup() == -1 )
        exit( 1 );
    pinMode( DHT_PIN, INPUT );
	
    uint8_t laststate    = HIGH;
	laststate = digitalRead( DHT_PIN );
	//printf( "Water = %d\n", laststate );
	if (LOW == laststate)
		printf( "Water: YES!\n");
	else if (HIGH == laststate)
		printf( "Water: NO\n");
  
    return(0);
}
