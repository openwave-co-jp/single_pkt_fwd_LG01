/*
  LoRaWAN packet forwarder example :
  Support Devices: LG01 Single Channel LoRa Gateway
  Require Library:
  RadioHead Library: https://github.com/dragino/RadioHead 

  LG01 test with firmware version v4.2.3
  
  Example sketch showing how to get LoRaWAN packets from LoRaWAN node and forward it to LoRaWAN Server
  It is designed to work with 
  modified 27 Jul 2017
  by Dragino Tech <support@dragino.com>
*/

#include <FileIO.h>
#include <Console.h> 
#include <Process.h>
#include <SPI.h>
#include <LoRa.h>
#include <avr/wdt.h>

static char packet[256];
static char message[256];

const String Sketch_Ver = "single_pkt_fwd_v002";

//Set Debug = 1 to enable Console Output;
const int debug = 0; 

static float rxfreq, txfreq;
static int SF, CR;
static long BW, preLen;

long rxsend_wait = 2000; // End-node channel of receive change time (for wait)
long chchanl_time = 10; // Time for RX1 to RX2 channel change.
const long sendpkt_time = 500;  // Time for Send packet time (RX1 and RX2).
const long cstime = 10; // Carrie Sense time

void getRadioConf();//Get LoRa Radio Configure from LG01
void setLoRaRadio();//Set LoRa Radio
void receivepacket();// receive packet
void sendpacket();

static int send_mode = 0; /* define mode default receive mode */

void setup() {

  if ( debug == 1 ) {
    rxsend_wait = rxsend_wait / 2;
  }

  // Setup Bridge 
  Bridge.begin(115200);
    
  // Setup File IO
  FileSystem.begin();
  
  Console.begin();
  //Print Current Version 
  Console.println("Start Simple packet forwarder.");
  //write sketch version to Linux
  writeVersion();
  Console.print("Sketch Version:");
  Console.println(Sketch_Ver);

  //Get 
  getRadioConf();
  
  Console.print(F("RX Frequency: "));
  Console.println(rxfreq);
  Console.print(F("TX Frequency: "));
  Console.println(txfreq);
  Console.print(F("Spread Factor: SF"));
  Console.println(SF);   
  Console.print(F("Coding Rate: 4/"));
  Console.println(CR);  
  Console.print(F("Bandwidth: "));
  Console.println(BW);  
  Console.print(F("PreambleLength: "));
  Console.println(preLen);   

  if (!LoRa.begin(rxfreq)) Console.println(F("init LoRa failed"));
  
  setLoRaRadio(); // Set LoRa Radio to Semtech Chip
  
  delay(1000);
  
}

void loop(){

  if (!send_mode) {   
    receivepacket();          /* received message and wait server downstream */
  } else {
    sendpacket();
  }
  
}

//Get LoRa Radio Configure from LG01
void getRadioConf() {
  //Read frequency from uci ####################
  int j = 0;
  
  memset(packet, 0, sizeof(packet));
  
  Process p;    // Create a process and call it "p"
  p.begin("uci");
  p.addParameter("get"); 
  p.addParameter("lorawan.radio.rx_frequency");
  p.run();    // Run the process and wait for its termination
  while (p.available() > 0 && j < 9) {   
   packet[j] = p.read();
   j++;
  }
  rxfreq = atof(packet);
  p.close();

  j=0;
  memset(packet, 0, sizeof(packet));
  p.begin("uci");
  p.addParameter("get"); 
  p.addParameter("lorawan.radio.tx_frequency");
  p.run();    // Run the process and wait for its termination
  while (p.available() > 0 && j < 10) {   
   packet[j]= p.read();
   j++;
  }
  txfreq = atof(packet); 
  p.close();

  //Read Spread Factor ####################
  j = 0;
  memset(packet, 0, sizeof(packet));
  p.begin("uci");
  p.addParameter("get"); 
  p.addParameter("lorawan.radio.SF");
  p.run();    // Run the process and wait for its termination
  while (p.available() > 0 && j < 3) {   
   packet[j] = p.read();
   j++;
  }
  
  SF = atoi(packet) > 0 ? atoi(packet) : 7;
  p.close();

  //Read Coding Rate  ####################
  j = 0;
  memset(packet, 0, sizeof(packet));
  p.begin("uci");
  p.addParameter("get"); 
  p.addParameter("lorawan.radio.coderate");
  p.run();    // Run the process and wait for its termination
  while (p.available() > 0 && j < 2) {   
   packet[j] = p.read();
   j++;
  }
  CR = atoi(packet);
  p.close();

  //Read PreambleLength
  j = 0;
  memset(packet, 0, sizeof(packet));
  p.begin("uci");
  p.addParameter("get"); 
  p.addParameter("lorawan.radio.preamble");
  p.run();    // Run the process and wait for its termination
  while (p.available() > 0 && j < 5) {   
   packet[j] = p.read();
   j++;
  }
  preLen = atol(packet);
  p.close();

  //Read BandWidth  #######################

  j = 0;
  memset(packet, 0, sizeof(packet));

  p.begin("uci");
  p.addParameter("get"); 
  p.addParameter("lorawan.radio.BW");
  p.run();    // Run the process and wait for its termination
  while (p.available() > 0 && j < 2) {   
   packet[j] = p.read();
   j++;
  }
  
  switch (atoi(packet)) {
    case 0:BW=7.8E3;break;
    case 1:BW=10.4E3;break;
    case 2:BW=15.6E3;break;
    case 3:BW=20.8E3;break;
    case 4:BW=31.25E3;break;
    case 5:BW=41.7E3;break;
    case 6:BW=62.5E3;break;
    case 7:BW=125E3;break;
    case 8:BW=250E3;break;
    case 9:BW=500E3;break;
    default:BW=125E3;break;
  }

  p.close();

}

void setLoRaRadio(){

    //SF12 correspond
    if (SF == 12) {
      if ( debug > 0 ) {
        Console.println("SF12.");
      }
      LoRa.writeRegister(0x0c, 0x23); 
      LoRa.writeRegister(0x1d, 0x72); 
      LoRa.writeRegister(0x1e, 0xc4); 
      LoRa.writeRegister(0x1f, 0x05); 
      LoRa.writeRegister(0x26, 0x0c); 
      LoRa.writeRegister(0x23, 0x40); 
    } else {
      if ( debug > 0 ) {
        Console.println("Not SF12.");
      }
      LoRa.setSpreadingFactor(SF); 
      LoRa.setSignalBandwidth(BW); 
      LoRa.setCodingRate4(CR); 

    }

    LoRa.setSyncWord(0x34); 
    LoRa.setPreambleLength(preLen); 
    LoRa.setTxPower(13);
    LoRa.receive(0);     
     
}

//Receiver LoRa packets and forward it 
void receivepacket() {
  // try to parse packet
  LoRa.setFrequency(rxfreq);
  setLoRaRadio();

  while(1) {

    int packetSize = LoRa.parsePacket();
     
    if (packetSize) {
      // Received a packet
      if ( debug > 0 ) {
          Console.println();
          Console.print(F("Get Packet:"));
          Console.print(packetSize);
          Console.println(F(" Bytes"));
      }
    
      // read packet
      int i = 0;
      
      memset(message, 0, sizeof(message)); /* make sure message is empty */

      while (LoRa.available() && i < 256) {
        message[i] = LoRa.read();       
        i++;  
      }    /* end of while lora.available */
        
      if ( debug > 0 )  {     
        for ( int debCnt = 0; debCnt < i; debCnt++) {
          Console.print("[");
          Console.print(debCnt);
          Console.print("]");
          Console.print(message[debCnt],HEX);
          Console.print(" ");
        }
      }

      if ( debug > 0 ) Console.println("");

      /*cfgdata file will be save rssi and packetsize*/
      File cfgFile = FileSystem.open("/var/iot/cfgdata", FILE_WRITE); 
      cfgFile.print("rssi=");
      cfgFile.println(LoRa.packetRssi()); 
      cfgFile.print("size=");
      cfgFile.println(packetSize);
      cfgFile.close();

      File dataFile = FileSystem.open("/var/iot/data", FILE_WRITE);
      for (int j = 0; j < i; j++) {
          dataFile.print(message[j]);
      }
      dataFile.close(); 

      if (message[0] == 0) {
        
        if ( debug > 0 ) Console.println("Receive join Request.");

        File oFile = FileSystem.open("/var/iot/dldata", FILE_WRITE); /* empty the dldata file */
        oFile.close();

        send_mode = 1;  /* change the mode */
        
      } else {
        if ( debug > 0 ) Console.println("Receive UP Link");
      }
      
      return; /* exit the receive loop after received data from the node */    

    } /* end of if packetsize than 1 */
  } /* end of recive loop */
}

void sendpacket()
{
  
  int i = 0;

  long old_time = millis();
  
  long new_time = old_time;  

  int respFlg = 0;

  // recieve join response.
  while(new_time - old_time < 10000) { /* received window may be closed after 5 seconds */

    new_time = millis();
            
    File dlFile = FileSystem.open("/var/iot/dldata"); /* dldata file save the downstream data */

    memset(packet, 0, sizeof(packet));
    i = 0;
    
    while (dlFile.available() && i < 256) {
      packet[i] = dlFile.read();
      i++;
    }
    
    dlFile.close();
    
    // If not receive join result from server, wait for response.
    if(i < 32) {
      continue; 
    } else {
      if ( debug > 0 ) {
        int j;
        Console.println(F("Downlink Message:"));
        for (j = 0; j < i; j++) {
            Console.print("[");
            Console.print(j);
            Console.print("]");
            Console.print(packet[j],HEX);
            Console.print(" ");
        }
        Console.println();
      }

      respFlg = 1;
      break;
      
    }
    
  }

  if (respFlg == 0) {
    if (debug > 0) Console.println(F("Can't get Server Response."));
    send_mode = 0;
    return 9;
  } else {
    Console.println("Get Server Response.");
  }
  
  // wait for open the end-node channel 
  delay(rxsend_wait);
  Console.println("Begin send down packet.");

  // Send lora packet to end node.
  new_time = millis();
  old_time = new_time;

  Console.println(F("[transmit] RX1 Start."));
  while(new_time - old_time < sendpkt_time) {

    if ( carrierSense() != 0 ) {
      continue;
    }

    LoRa.beginPacket();
    LoRa.print(packet);
    LoRa.endPacket();
    
    if(debug > 0) {
      Console.print(new_time - old_time, DEC);
      Console.println(F(":[transmit] rxfreq"));
    }

    delay(10);

    new_time = millis();

  }
  Console.println(F("[transmit] RX1 End."));

  // wait for open the end-node channel 
  delay(chchanl_time);

  if (SF != 9) {

    new_time = millis();
    old_time = new_time;

    LoRa.setFrequency(txfreq);
    LoRa.setSpreadingFactor(9);    /* begin send data to the lora node, lmic use the second receive window, and SF default to 9 */
    delay(2);

    Console.println(F("[transmit] RX2 Start."));
    while(new_time - old_time < sendpkt_time) {

      if ( carrierSense() != 0 ) {
        continue;
      }
      
      LoRa.beginPacket();
      LoRa.print(packet);
      LoRa.endPacket();

      if(debug > 0) {
        Console.print(new_time - old_time, DEC);
        Console.println(F(":[transmit] txfreq"));
      }

      delay(10);
      
      new_time = millis();
      
    }
    
  }

  Console.println(F("[transmit] RX2 END"));

  send_mode = 0;

}

int carrierSense() {
  int ret = 0;
  // CS
  LoRa.receive(0);
  delay(cstime);
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    ret = 9;
  }  else {
    ret = 0;
  }
  return ret;
}

//Function to write sketch version number into Linux. 
void writeVersion()
{
  File fw_version = FileSystem.open("/var/avr/fw_version", FILE_WRITE);
  fw_version.print(Sketch_Ver);
  fw_version.close();
}
