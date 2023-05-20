/*
 ntripClient V 1.0
 based on Example16_NTRIPClient_WithGGA
 By: SparkFun Electronics / Nathan Seidle
  Date: November 18th, 2021
  License: MIT. See license file for more information.

 */


#include <SPI.h>
#include <EthernetENC.h>

#include "base64_utils.h"
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress server(192, 168, 6, 208);   // Set the rosserial socket ROSCORE SERVER IP address (change with your IP)
const uint16_t serverPort = 11411;    // Set the rosserial socket server port

#define ROSSERIAL_ARDUINO_TCP
#include <ros.h>
#include <std_msgs/String.h>
#include <rtcm_msgs/Message.h>.       // the rtcm topic format
ros::NodeHandle nh;

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
long lastReceivedRTCM_ms = 0; //5 RTCM messages take approximately ~300ms to arrive at 115200bps
int maxTimeBeforeHangup_ms = 10000; //If we fail to get a complete RTCM frame after 10s, then disconnect from caster
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
bool transmitLocation = true;        //By default we will transmit the units location via GGA sentence.
int timeBetweenGGAUpdate_ms = 10000; //GGA is required for Rev2 NTRIP casters. Don't transmit but once every 10 seconds
long lastTransmittedGGA_ms = 0;

//Used for GGA sentence parsing from incoming NMEA
bool ggaSentenceStarted = false;
bool ggaSentenceComplete = false;
bool ggaTransmitComplete = false; //Goes true once we transmit GGA to the caster

char ggaSentence[128] = "$GPGGA,111452.559,4225.436,N,01152.952,E,1,12,1.0,0.0,M,0.0,M,,*6C"; // change according your gps position
byte ggaSentenceSpot = 0;
int ggaSentenceEndSpot = 0;
char casterHost[] = "*******"; // ip of caster server
int casterPort = 2101;
char mountPoint[] = "******"; // mountpoint
char casterUser[] = "";       // user
char casterUserPW[] = "";     // password 
// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

// Variables to measure the speed
unsigned long beginMicros, endMicros;
unsigned long byteCount = 0;
std_msgs::String str_msg;

rtcm_msgs::Message rtcm;
ros::Publisher rtcm_msg ("/ublox_gps/rtcm", &rtcm); // change the name of topic 
void setup() {
  // You can use Ethernet.init(pin) to configure the CS pin
  Ethernet.init(53);  // the pin for Arduino Mega 
  nh.getHardware()->setConnection(server, serverPort);
  nh.initNode();
  nh.advertise(rtcm_msg);
  rtcm.message = (uint8_t *) malloc(sizeof(uint8_t)*50);
  rtcm.header.seq = 0;
  delay(2000);
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while (true) {
        delay(1); // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);





  
  

  // if you get a connection, report back via serial:
  
  beginMicros = micros();
}
void beginClient()
{
  EthernetClient ntripClient;
  long rtcmCount = 0;

  Serial.println(F("Subscribing to Caster. Press key to stop"));
  delay(10); //Wait for any serial to arrive
  while (Serial.available())
    Serial.read(); //Flush

  while (Serial.available() == 0)
  {

    //Connect if we are not already. Limit to 5s between attempts.
    if (ntripClient.connected() == false)
    {
      Serial.print(F("Opening socket to "));
      Serial.println(casterHost);

      if (ntripClient.connect(casterHost, casterPort) == false) //Attempt connection
      {
        Serial.println(F("Connection to caster failed"));
        return;
      }
      else
      {
        Serial.print(F("Connected to "));
        Serial.print(casterHost);
        Serial.print(F(": "));
        Serial.println(casterPort);

        Serial.print(F("Requesting NTRIP Data from mount point "));
        Serial.println(mountPoint);

        const int SERVER_BUFFER_SIZE = 512;
        char serverRequest[SERVER_BUFFER_SIZE + 1];

        snprintf(serverRequest,
                 SERVER_BUFFER_SIZE,
                 "GET /%s HTTP/1.0\r\nUser-Agent: NTRIP SparkFun u-blox Client v1.0\r\n",
                 mountPoint);

        char credentials[512];
        if (strlen(casterUser) == 0)
        {
          strncpy(credentials, "Accept: */*\r\nConnection: close\r\n", sizeof(credentials));
        }
        else
        {
          //Pass base64 encoded user:pw
          char userCredentials[sizeof(casterUser) + sizeof(casterUserPW) + 1]; //The ':' takes up a spot
          snprintf(userCredentials, sizeof(userCredentials), "%s:%s", casterUser, casterUserPW);

          Serial.print(F("Sending credentials: "));
          Serial.println(userCredentials);

#if defined(ARDUINO_ARCH_ESP32)
          //Encode with ESP32 built-in library
          base64 b;
          String strEncodedCredentials = b.encode(userCredentials);
          char encodedCredentials[strEncodedCredentials.length() + 1];
          strEncodedCredentials.toCharArray(encodedCredentials, sizeof(encodedCredentials)); //Convert String to char array
#else
          snprintf((char *)userCredentials, sizeof(userCredentials), "%s:%s", casterUser, casterUserPW);

          char encodedCredentials[sizeof(userCredentials)+1];
          b64_encode(encodedCredentials, userCredentials,sizeof(userCredentials));
          snprintf(credentials, sizeof(credentials), "Authorization: Basic %s\r\n", encodedCredentials);
#endif

          snprintf(credentials, sizeof(credentials), "Authorization: Basic %s\r\n", encodedCredentials);
        }
        strncat(serverRequest, credentials, SERVER_BUFFER_SIZE);
        strncat(serverRequest, "\r\n", SERVER_BUFFER_SIZE);

        Serial.print(F("serverRequest size: "));
        Serial.print(strlen(serverRequest));
        Serial.print(F(" of "));
        Serial.print(sizeof(serverRequest));
        Serial.println(F(" bytes available"));

        Serial.println(F("Sending server request:"));
        Serial.println(serverRequest);
        ntripClient.write(serverRequest, strlen(serverRequest));

        //Wait for response
        unsigned long timeout = millis();
        while (ntripClient.available() == 0)
        {
          if (millis() - timeout > 5000)
          {
            Serial.println(F("Caster timed out!"));
            ntripClient.stop();
            return;
          }
          delay(10);
        }

        //Check reply
        bool connectionSuccess = false;
        char response[512];
        int responseSpot = 0;
        while (ntripClient.available())
        {
          if (responseSpot == sizeof(response) - 1)
            break;

          response[responseSpot++] = ntripClient.read();
          if (strstr(response, "200") != nullptr) //Look for '200 OK'
            connectionSuccess = true;
          if (strstr(response, "401") != nullptr) //Look for '401 Unauthorized'
          {
            Serial.println(F("Hey - your credentials look bad! Check you caster username and password."));
            connectionSuccess = false;
          }
        }
        response[responseSpot] = '\0';

        Serial.print(F("Caster responded with: "));
        Serial.println(response);

        if (connectionSuccess == false)
        {
          Serial.print(F("Failed to connect to "));
          Serial.println(casterHost);
          return;
        }
        else
        {
          Serial.print(F("Connected to "));
          Serial.println(casterHost);
          lastReceivedRTCM_ms = millis(); //Reset timeout
          ggaTransmitComplete = true;     //Reset to start polling for new GGA data
          Serial.print(F("Pushing GGA to server: "));
          Serial.println(ggaSentence);     //debug

      lastTransmittedGGA_ms = millis();

      //Push our current GGA sentence to caster
      ntripClient.print(ggaSentence);
      ntripClient.print("\r\n");

      ggaTransmitComplete = true;
        }
      } //End attempt to connect
    }   //End connected == false

    if (ntripClient.connected() == true)
    {
      uint8_t rtcmData[512 * 4]; //Most incoming data is around 500 bytes but may be larger
      rtcmCount = 0;

      //Print any available RTCM data
      while (ntripClient.available())
      {
        //Serial.write(ntripClient.read()); //Pipe to serial port is fine but beware, it's a lot of binary data
        rtcmData[rtcmCount++] = ntripClient.read();
        if (rtcmCount == sizeof(rtcmData))
          break;
      }

      if (rtcmCount > 0)
      {
        lastReceivedRTCM_ms = millis();

       
       uint8_t * np = (uint8_t *) realloc(rtcm.message,sizeof(uint8_t)*rtcmCount);
       if( np != NULL) { rtcm.message = np; }
       rtcm.header.seq += 1;
       rtcm.header.stamp = nh.now();
       rtcm.message_length=rtcmCount;
        uint8_t *p1 =rtcm.message;
        uint8_t *p2=rtcmData ;
       for (int i = 0;i<rtcmCount;i++) *p1++ = *p2++;
       rtcm_msg.publish(&rtcm);
       
       nh.spinOnce();
      }
    }

    //Provide the caster with our current position as needed
    if (ntripClient.connected() == true && transmitLocation == true && (millis() - lastTransmittedGGA_ms) > timeBetweenGGAUpdate_ms && ggaSentenceComplete == true && ggaTransmitComplete == false)
    {
      Serial.print(F("Pushing GGA to server: "));
      Serial.println(ggaSentence);

      lastTransmittedGGA_ms = millis();

      //Push our current GGA sentence to caster
      ntripClient.print(ggaSentence);
      ntripClient.print("\r\n");

      ggaTransmitComplete = true;
    }

    //Close socket if we don't have new data for 10s
    if (millis() - lastReceivedRTCM_ms > maxTimeBeforeHangup_ms)
    {
      Serial.println(F("RTCM timeout. Disconnecting..."));
      if (ntripClient.connected() == true)
        ntripClient.stop();
      return;
    }

    delay(10);
  }
  ntripClient.stop();
}
long lastTime = 0; 
void loop() {
  
  beginClient();
  if (millis() - lastTime > 1000)
  {
    lastTime = millis(); //Update the timer    
  }
  delay(10000);
}
