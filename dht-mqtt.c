/*
 *  dht.c:
 *    read temperature and humidity from DHT11 or DHT22 sensor
 * Compile: gcc -pthread -odht-mqtt dht-mqtt.c -L/usr/local/lib -lwiringPi  -lpaho-mqtt3c
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "string.h"
#include "MQTTClient.h"

#define MAX_TIMINGS    85
#define DHT_PIN        7

//#define MQTT_ADDRESS     "tcp://test.mosquitto.org:1883"
#define MQTT_ADDRESS     "tcp://localhost:1883"
#define MQTT_CLIENTID    "ExampleClientPub"
#define MQTT_TOPIC       "my_topic88888"
#define MQTT_PAYLOAD     "Hello World!"
#define MQTT_QOS         1
#define MQTT_TIMEOUT     10000L

int mqtt_send(char* sTopic, char* sPayload)
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    MQTTClient_create(&client, MQTT_ADDRESS, MQTT_CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
    pubmsg.payload = sPayload;
    pubmsg.payloadlen = strlen(sPayload);
    pubmsg.qos = MQTT_QOS;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(client, sTopic, &pubmsg, &token);
    printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(MQTT_TIMEOUT/1000), sPayload, sTopic, MQTT_CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, MQTT_TIMEOUT);
    printf("Message with delivery token %d delivered\n", token);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

int data[5] = { 0, 0, 0, 0, 0 };

int read_dht_data(int iValidPacketCounter)
{
    uint8_t laststate    = HIGH;
    uint8_t counter        = 0;
    uint8_t j            = 0, i;

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    /* pull pin down for 18 milliseconds */
    pinMode( DHT_PIN, OUTPUT );
    digitalWrite( DHT_PIN, LOW );
    delay( 18 );

    /* prepare to read the pin */
    pinMode( DHT_PIN, INPUT );

    /* detect change and read data */
    for ( i = 0; i < MAX_TIMINGS; i++ )
    {
        counter = 0;
        while ( digitalRead( DHT_PIN ) == laststate )
        {
            counter++;
            delayMicroseconds( 1 );
            if ( counter == 255 )
            {
                break;
            }
        }
        laststate = digitalRead( DHT_PIN );

        if ( counter == 255 )
            break;

        /* ignore first 3 transitions */
        if ( (i >= 4) && (i % 2 == 0) )
        {
            /* shove each bit into the storage bytes */
            data[j / 8] <<= 1;
            if ( counter > 16 )
                data[j / 8] |= 1;
            j++;
        }
    }

    /*
     * check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
     * print it out if data is good
     */
    if ( (j >= 40) &&
         (data[4] == ( (data[0] + data[1] + data[2] + data[3]) & 0xFF) ) )
    {
        float h = (float)((data[0] << 8) + data[1]) / 10;
        if ( h > 100 )
        {
            h = data[0];    // for DHT11
        }
        float c = (float)(((data[2] & 0x7F) << 8) + data[3]) / 10;
        if ( c > 125 )
        {
            c = data[2];    // for DHT11
        }
        if ( data[2] & 0x80 )
        {
            c = -c;
        }
        float f = c * 1.8f + 32;
                if (0 != iValidPacketCounter) { // First reading is not valid - skipping it
                        printf( "Humidity = %.1f %% Temperature = %.1f *C (%.1f *F)\n", h, c, f );
			char BUFFER[255];
			sprintf(BUFFER, "Humidity = %.1f %% Temperature = %.1f *C (%.1f *F)", h, c, f );
			//mqtt_send("my_topic88888", BUFFER);
			sprintf(BUFFER, "%.1f", h );
			mqtt_send("/DHT1/Humidity", BUFFER);
			sprintf(BUFFER, "%.1f", c );
			mqtt_send("/DHT1/Temperature", BUFFER);
		}
                return 1;
    }else  {
        //printf( "Data not good, skip\n" );
    }
        return 0;
}

int main( int argc, char *argv[] )
{
    //printf( "Raspberry Pi DHT11/DHT22 temperature/humidity test\n" );

    if ( wiringPiSetup() == -1 )
        exit( 1 );

        int iValidPacketCounter = 0;
    while ( 1 )
    {
        iValidPacketCounter += read_dht_data(iValidPacketCounter);
        if (2 == argc && iValidPacketCounter > 1) break; // Single shot if any command line argument supplied
        delay( 60000 ); /* wait 60 seconds before next read */
    }

    return(0);
}

