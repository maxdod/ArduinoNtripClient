# ArduinoNtripClient
An Arduino Ntrip client to dispatch rtcm ros topic for Ublox fp9 gps Rtk correction

I have a project where three rovers move autonomously. Relative location must be accurate (below 10 cm). It is essential to provide the RTK correction to the three gps receivers. The three rovers are connected to a local network via long-range wifi. An arduino sends the corrections via a caster server with ntrip protocol to all gps devices. The messages are encapsulated in the rtcm ros topic.



Use Arduino Mega 2560 and ethernet adapter based on ENC28J60.

Requires arduino library:   https://github.com/JAndrassy/EthernetENC

Produces a ros topic /rtcm thru a connection to rosserial_server socket node

To setup: 

1) change the caster name, the mountpoint and the credentials.

char casterHost[] = "*******"; // ip or name of caster server

int casterPort = 2101;         // usually this

char mountPoint[] = "******"; // mountpoint (depends by the caster)

char casterUser[] = "----";       // user

char casterUserPW[] = "----";     // password 

2) Change the ggaSentence based on you position.
Many Caster servers require this data to send back the corrections:
The caster send the corrections by the station nearest to you in order to maximize the precision.

To generate the string based on your position you can use this web site: https://www.nmeagen.org

char ggaSentence[128] = "$GPGGA,111452.559,4225.436,N,01152.952,E,1,12,1.0,0.0,M,0.0,M,,*6C";



3) set the address of rosserial_server socket

IPAddress server(192, 168, 6, 208);   // Change the rosserial socket ROSCORE SERVER IP address
const uint16_t serverPort = 11411;    // Set the rosserial socket server port

          
4) on the computer where Ros is running launch:
          
 roslaunch rosserial_server socket.launch to start the socket server

5) I assume that ublox launch file is running

for Ublox gps device use the following drivers: https://github.com/ros-agriculture/ublox_f9p that support rtcm corrections

The Arduino & ethernet plug :

![IMG_7922](https://github.com/maxdod/ArduinoNtripClient/assets/39596051/a0ced7e8-87bc-4e2a-a360-8055bf8ea5c6)

Arduino pin connection
![schema](https://github.com/maxdod/ArduinoNtripClient/assets/39596051/1a6b2d00-a6c6-4755-b756-d12bca8a1f75)
