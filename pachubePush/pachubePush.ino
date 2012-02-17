/*
  Pachube sensor client with Strings
 
 This sketch connects an analog sensor to Pachube (http://www.pachube.com)
 using a Wiznet Ethernet shield. You can use the Arduino Ethernet shield, or
 the Adafruit Ethernet shield, either one will work, as long as it's got
 a Wiznet Ethernet module on board.
 
 This example uses the String library, which is part of the Arduino core from
 version 0019.  
 
 Circuit:
 * Analog sensor attached to analog in 0
 * Button attached to pin 7
 * LED attached to pin 9
 * Ethernet shield attached to pins 10, 11, 12, 13
 
 based on code
 created 15 March 2010
 updated 4 Sep 2010
 by Tom Igoe
 
 This code is in the public domain.
 
 */

#include <SPI.h>
#include <Ethernet.h>

// assign a MAC address for the ethernet controller.
// fill in your address here:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// fill in an available IP address on your network here,
// for manual configuration:
IPAddress ip(10,0,1,20);

// institution specific Pachube feed address
String feedAddr = "45999";  //http://pachube.com/feeds/45999

// your API key
String ApiKey = "5VCx3ftgRHd6AT2leyay2ySmARPb9ZUsHY-KgtgTLoLKMg0VEqjxYIbRo0KAItkWXOlnDSdQyECjAaQ9Vb307npyLewgxEp3JE8yUN8YdR-mAIMB0KM8MuploVzpVt6T";

// initialize the library instance:
EthernetClient client;

long lastConnectionTime = 0;        // last time you connected to the server, in milliseconds
boolean lastConnected = false;      // state of the connection last time through the main loop
const int postingInterval = 10000;  //delay between updates to Pachube.com

String dataString;

const int buttonPin = 7;
const int ledPin = 9;
int currButtonVal = 0;
int prevButtonVal = 0;
int myDigitalVal = 0;
int myAnalogVal = 0;

void setup() {
  pinMode(buttonPin, INPUT);  //button
  pinMode(ledPin, OUTPUT); //LED

  // start the ethernet connection and serial port:
  Serial.begin(9600);
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Configure manually:
    Ethernet.begin(mac, ip);
  }
  // give the ethernet module time to boot up:
  delay(1000);
}

void loop() {

  //--- button read
  currButtonVal = digitalRead(buttonPin);  //read our button
  if(currButtonVal != prevButtonVal){ //check for state change
    prevButtonVal = currButtonVal;
    if(prevButtonVal == 1){ 
      myDigitalVal ++ ;           //add one on each press
      if(myDigitalVal > 1){       //count between 0 and 1.
        myDigitalVal = 0;
      }
    }
  }
  digitalWrite(ledPin, myDigitalVal);    //turn LED on/off to represent myDigitalVal

  //--- sensor read
  int analogSensor = analogRead(A0);   
  //int analogSensor = random(1023); //randomize

  // convert the data to a String to send it:
  dataString = String(analogSensor);

  // you can append multiple readings to this String if your
  // pachube feed is set up to handle multiple values:
  dataString += ",";
  dataString += String(myDigitalVal);

  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    Serial.println("disconnecting.");
    client.stop();
  }

  // if you're not connected, and ten seconds have passed since
  // your last connection, then connect again and send data:
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    Serial.println("\n------- SEND DATA -------"); //some serial monitor feedback
    Serial.print("analog, digital = ");
    Serial.println(dataString);
    Serial.println("-------------------------\n");

    sendData(dataString); //SEND!
  }
  // store the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
}

// this method makes a HTTP connection to the server:
void sendData(String thisData) {

  // if there's a successful connection:
  if (client.connect("api.pachube.com", 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request. 
    client.print("PUT /v2/feeds/" + feedAddr + ".csv HTTP/1.1\n");
    client.print("Host: api.pachube.com\n");
    // fill in your Pachube API key here:
    client.print("X-PachubeApiKey: " + ApiKey + "\n");
    client.print("Content-Length: ");
    client.println(thisData.length(), DEC);

    // last pieces of the HTTP PUT request:
    client.print("Content-Type: text/csv\n");
    client.println("Connection: close\n");

    // here's the actual content of the PUT request:
    client.println(thisData);

    // note the time that the connection was made:
    lastConnectionTime = millis();
  } 
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
}






