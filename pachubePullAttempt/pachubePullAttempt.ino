/*
  Web client
 
 This sketch connects to a website (http://www.google.com)
 using an Arduino Wiznet Ethernet shield. 
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 
 created 18 Dec 2009
 by David A. Mellis
 
 */

#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {  
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
//IPAddress server(173,194,33,104); // Google

String feedAddr = "45999";  //https://pachube.com/feeds/45999
// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):
Client client;

unsigned long int currTime = 0;
unsigned long int timer = 0;

void setup() {
  // start the serial library:
  Serial.begin(9600);
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);

}

void loop() {
  currTime = millis();

  if(currTime > timer){
    timer = currTime + 5000;
    getData();
    // if there are incoming bytes available 
    // from the server, read them and print them:
    if (client.available()) {
      char c = client.read();
      Serial.print(c);
    }

  }


  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();

    // do nothing forevermore:
   // for(;;)
    //  ;
  }
}

void getData(){

  Serial.println("connecting...");

  // if you get a connection, report back via serial:
  if (client.connect("www.pachube.com", 80)) {
    Serial.println("connected");
    // Make a HTTP request:
    client.println("GET /v2/feeds/" + feedAddr + ".xml HTTP/1.1\n");
    client.print("Host: api.pachube.com\n");
    // fill in your Pachube API key here:
    client.print("X-PachubeApiKey: 5VCx3ftgRHd6AT2leyay2ySmARPb9ZUsHY-KgtgTLoLKMg0VEqjxYIbRo0KAItkWXOlnDSdQyECjAaQ9Vb307npyLewgxEp3JE8yUN8YdR-mAIMB0KM8MuploVzpVt6T\n");
    //client.print("Content-Length: ");
    client.println();
  } 
  else {
    // kf you didn't get a connection to the server:
    Serial.println("connection failed");
  }
}




