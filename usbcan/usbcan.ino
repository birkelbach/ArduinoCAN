/*  USB<->CAN Interface for the Arduino 
 *  Copyright (c) 2013 Phil Birkelbach
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <SPI.h>
#define CAN_MHZ 20

#include "can.h"

#define PIN_SS 10
#define RXBUFF_SIZE 32

CAN Can((byte)PIN_SS);
char rxbuff[RXBUFF_SIZE];
byte rxbuffidx;
word id;

byte buff[16];

inline void sendHelp(void)
{
  Serial.print("CAN-FIXit USB to CAN Interface Adapter\n");
  Serial.print("Version 1.0\n\n");
  Serial.print("  Command            Code\n");
  Serial.print("-------------------------\n");
  Serial.print("  Reset               K\n");
  Serial.print("  Set Bitrate         B\n");
  Serial.print("  Send Frame          W\n");
  Serial.print("  Open Port           O\n");
  Serial.print("  Close Port          C\n");
  Serial.print("  Set Mask            M\n");
  Serial.print("  Set Filter          F\n");
  Serial.print("  MCP2515 Raw Message Z\n");
  Serial.print("  Help                H\n");
}

inline void setBitrate(void)
{
  byte mode;
  mode = Can.getMode();
  if(mode == MODE_NORMAL) {
    Can.setMode(MODE_CONFIG);
  }
  if(strncmp(rxbuff, "B125\n", 5) == 0) {
    Can.setBitRate(125);
  } else if(strncmp(rxbuff, "B250\n", 5) == 0) {
    Can.setBitRate(250);
  } else if(strncmp(rxbuff, "B500\n", 5) == 0) {
    Can.setBitRate(500);
  } else if(strncmp(rxbuff, "B1000\n", 6) == 0) {
    Can.setBitRate(500);
  } else {
    Serial.print("*2\n");
    return;
  }
  if(mode == MODE_NORMAL) {
    Can.setMode(MODE_NORMAL);
  }
  Serial.print("b\n");
}

/* Get's the ID from the string.  The ids argument should
   be a pointer to a four byte array that is assumed to be
   SIDH, SIDL, EID8 and EID0 in that order.  The buff
   argument should be a pointer to the first character
   in rxbuff where the id, filter or mask is located
   This function will fill those out ids and return 
   0x00 if it all works, otherwise it'll return nonzero */
byte getID(char *buff, byte *ids)
{
  long id;
  if(buff[3]==':' || buff[3]=='\n') { //Standard frame
    Serial.print("Standard Frame");
    buff[3] = 0;
    id = strtoul(&buff[0], NULL, 16);
    ids[0] = (id & 0x00000FFFL) >> 3;
    ids[1] = (id & 0x00000007L) << 5;
    ids[2] = 0;
    ids[3] = 0;
    
  } else if(buff[8]==':' || buff[8] == '\n') { //Extended frame
    Serial.print("Extended Frame\n");
    buff[8] = 0;
    id = strtoul(&buff[0], NULL, 16);
    ids[0] = (id & 0x00000FFFL) >> 3;
    ids[1] = (id & 0x00000007L) << 5;
    ids[1] |= 0x08 | (id >> 27);
    ids[2] = id >> 19;
    ids[3] = id >> 11;    
  }
  Serial.print(id, HEX);
  Serial.print("\nSIDH ");
  Serial.print(ids[0], HEX);
  Serial.print("\n"); 
  Serial.print("SIDL ");
  Serial.print(ids[1], HEX);
  Serial.print("\n"); 
  Serial.print("EID8 ");
  Serial.print(ids[2], HEX);
  Serial.print("\n"); 
  Serial.print("EID0 ");
  Serial.print(ids[3], HEX);
  Serial.print("\n"); 
}

inline void writeFrame(void)
{
  word id;
  byte data[8], ids[4];
  byte length;

  if(getID(&rxbuff[1], ids)) {
    Serial.print("*1\n");
    return;
  }
  if(Can.getMode() != MODE_NORMAL) {
    Serial.print("*6\n");
  } else {
    ;
  }  
  Serial.print("w\n");
}

inline void writeFilter(void)
{
  byte mode, reg, ids[4];
  word filter;

  if(getID(&rxbuff[2], ids)) {
    Serial.print("*1\n");
    return;
  }
  mode = Can.getMode();
  if(mode == MODE_NORMAL) {
    Can.setMode(MODE_CONFIG);
  }

  if(mode == MODE_NORMAL) {
    Can.setMode(MODE_NORMAL);
  }
  Serial.print("f\n");
}

inline void writeMask(void)
{
  byte mode, ids[4];
  
  if(getID(&rxbuff[2], ids)) {
    Serial.print("*1\n");
    return;
  } 
  mode = Can.getMode();
  if(mode == MODE_NORMAL) {
    Can.setMode(MODE_CONFIG);
  }
  
  
  if(mode == MODE_NORMAL) {
    Can.setMode(MODE_NORMAL);
  }
  Serial.print("m\n");
}

/* Called if a full sentence has been received.  Determines
   what to do with the message */
inline void cmdReceived(void)
{
  if(rxbuff[0]=='O' && rxbuff[1] == '\n') {
    Can.setMode(MODE_NORMAL);
    Serial.print("o\n");
  } else if(rxbuff[0]=='C' && rxbuff[1] == '\n') {
    Can.setMode(MODE_CONFIG);
    Serial.print("c\n");
  } else if(rxbuff[0]=='K' && rxbuff[1] == '\n') {
    Can.sendCommand(CMD_RESET);
    Serial.print("k\n");
  } else if(rxbuff[0] == 'B') {
    setBitrate();
  } else if(rxbuff[0] == 'W') {
    writeFrame();
  } else if(rxbuff[0] == 'F') {
    writeFilter();
  } else if(rxbuff[0] == 'M') {
    writeMask();
  } else if(rxbuff[0] == 'H' && rxbuff[1] == '\n') {
    sendHelp();
  } else {
    Serial.print("*1\n");
  }
  // Just for testing...
  buff[0] = 0x00; // TXBxCTRL
  buff[1] = lowByte(id>>3);  // TXBxSIDH
  buff[2] = lowByte(id)<<5;// TXBxSIDL
  buff[3] = 0x00;
  buff[4] = 0x00;
  buff[5] = 0x02; // TXBxDLC
  buff[6]++;      // TXBxD0
  buff[7] = buff[6]+1;// TXBxD1
  Can.write(REG_TXB0, buff, 8);
  id++;
  if(id == 2048) { id = 0; }
  Can.sendCommand(CMD_RTS0);  
  Can.read(REG_CNF3, buff, 3);
}  

/* Prints a received frame to serial port in the proper format */
void printFrame(CanFrame frame)
{
  byte n;
  char str[9];
  Serial.print("r");
  if(frame.eid) sprintf(str, "%08lX", frame.id);
  else          sprintf(str, "%03X", frame.id); 
  Serial.print(str);
  Serial.print(":");
  for(n=0;n<frame.length;n++) {
    if(frame.data[n]<0x10) Serial.print("0");
    Serial.print(frame.data[n], HEX);
  }
  Serial.print("\n");
}


void setup() {
  Serial.begin(115200);
  Can.sendCommand(CMD_RESET);
  Can.setBitRate(125);
}

void loop()
{
  int ch;
  byte buff[13];
  byte result;
  byte length;
  unsigned long cid;
  CanFrame frame;
  
  if(Serial) {
    if(Serial.available()) {
      rxbuff[rxbuffidx] = Serial.read();
      if(rxbuff[rxbuffidx] == '\n') {
        cmdReceived();
        rxbuffidx = 0;
      } else {
        rxbuffidx++;
      }
      if(rxbuffidx == RXBUFF_SIZE) rxbuffidx = 0;
    }  
  }
  /* Thgis is where we check to see if we have received a frame
     on one of the Receive buffers.  */
  if(result = Can.getRxStatus()) {
    if(result & 0x40) {
      frame = Can.readFrame(0);
      printFrame(frame);
    }
    if(result & 0x80) {
      frame = Can.readFrame(1);
      printFrame(frame);
    }
  }
}



