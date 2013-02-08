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

void sendHelp(void)
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

void setBitrate(void)
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

void cmdReceived(void)
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


void setup() {
  Serial.begin(115200);
  Can.sendCommand(CMD_RESET);
  Can.setBitRate(125);
}

void loop()
{
  int ch;
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
  //delay(800);
}



