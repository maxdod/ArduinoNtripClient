# ArduinoNtripClient
An Arduino Ntrip client to dispatch rtcm ros topic for Ublox fp9 gps Rtk correction

I have a project where three rovers move autonomously. Relative location must be accurate (below 10 cm). It is essential to provide the RTK correction to the three gps receivers. The three rovers are connected to a local network via long-range wifi. An arduino sends the corrections via a caster server with ntrip protocol to all gps devices. The messages are encapsulated in the rtcm ros topic.



Use Arduino Mega 2560 and ethernet adapter based on ENC28J60
Requires arduino library:   https://github.com/JAndrassy/EthernetENC

Produces a ros topic /rtcm thru a connection to rosserial_server socket node

To setup: change the caster name, the mountpoint and the credentials.
          set the address of rosserial_server socket
          
          roslaunch rosserial_server socket.launch to start the socket server

for Ublox gps device use the following drivers: https://github.com/ros-agriculture/ublox_f9p that support rtcm corrections
![IMG_7922](https://github.com/maxdod/ArduinoNtripClient/assets/39596051/a0ced7e8-87bc-4e2a-a360-8055bf8ea5c6)

Arduino pin connection
![schema](https://github.com/maxdod/ArduinoNtripClient/assets/39596051/1a6b2d00-a6c6-4755-b756-d12bca8a1f75)
