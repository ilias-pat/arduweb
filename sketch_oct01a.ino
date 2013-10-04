#include <SPI.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <SD.h>

#include "WebServer.h"
#include "WebClient.h"

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1, 177);
WebServer serv( 80, "/" );

int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }  
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  Serial.print("free ram: ");
  Serial.println( freeRam( ) );
  
  serv.begin( );
}

void loop() {
  // put your main code here, to run repeatedly: 
  serv.process( );
}
