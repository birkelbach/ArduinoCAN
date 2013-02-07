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
byte rxbuff[RXBUFF_SIZE];
byte rxbuffidx;
word id;
byte mode;

byte buff[16];

void cmdReceived(void)
{
  if(rxbuff[0]=='O') {
    Can.setMode(MODE_NORMAL);
    Serial.print("o\n");
  } else if(rxbuff[0]=='C') {
    Can.setMode(MODE_CONFIG);
    Serial.print("c\n");
  }
  
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
  Can.setMode(MODE_NORMAL);
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



