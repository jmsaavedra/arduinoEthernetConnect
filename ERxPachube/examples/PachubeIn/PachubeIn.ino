/*
Pachube Data In

Demonstrates use of the ERxPachube library. 
Get remote sensor data from Pachube server.
If you don't have a Pachube account, register one first (http://www.pachube.com/).

To run this sketch, you need to:
1. Use your API key to replace the space holer PACHUBE_API_KEY below. 

In the serial monitor, you can get the data shown in http://api.pachube.com/v2/feeds/13747.csv.

Circuit:
* Ethernet shield attached to pins 10, 11, 12, 13

* Created 22 April 2011
* By Jeffrey Sun
* http://code.google.com/p/pachubelibrary/

*/
#include <Arduino.h>
#include <HardwareSerial.h>
#include <ERxPachube.h>
#include <Ethernet.h>
#include <SPI.h>

byte mac[] = { 0xCC, 0xAC, 0xBE, 0xEF, 0xFE, 0x91 }; // make sure this is unique on your network
byte ip[] = { 192, 168, 1, 177   };                  // no DHCP so we set our own IP address

#define PACHUBE_API_KEY				"PACHUBE_API_KEY" // fill in your API key PACHUBE_API_KEY
#define PACHUBE_FEED_ID				13747

ERxPachubeDataIn datain(PACHUBE_API_KEY, PACHUBE_FEED_ID);

void PrintDataStream(const ERxPachube& pachube);

void setup() {

	Serial.begin(9600);
	Ethernet.begin(mac, ip);
}

void loop() {
	Serial.println("+++++++++++++++++++++++++++++++++++++++++++++++++");
	int status = datain.syncPachube();

	Serial.print("sync status code <OK == 200> => ");
	Serial.println(status);

	PrintDataStream(datain);

	delay(5000);
}

void PrintDataStream(const ERxPachube& pachube)
{
	unsigned int count = pachube.countDatastreams();
	Serial.print("data count=> ");
	Serial.println(count);

	Serial.println("<id>,<value>");
	for(unsigned int i = 0; i < count; i++)
	{
		Serial.print(pachube.getIdByIndex(i));
		Serial.print(",");
		Serial.print(pachube.getValueByIndex(i));
		Serial.println();
	}
}