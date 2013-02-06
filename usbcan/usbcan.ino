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

void setup() {
  CAN.begin(PIN_SS);
  Serial.begin(115200);
  CAN.sendCommand(CMD_RESET);
  delay(500);
  CAN.setBitRate(125);
  delay(10);
  //digitalWrite(slaveSelectPin, LOW);  
  //SPI.transfer(CMD_WRITE);
  //SPI.transfer(REG_CANCTRL);
  //SPI.transfer(0x07);
  //digitalWrite(slaveSelectPin, HIGH);
  CAN.setMode(MODE_NORMAL);
}

byte buff[16];
word id;
 
void loop()
{
  delay(800);
  CAN.PrintRegister(REG_CANSTAT, "STAT");
  buff[0] = 0x00; // TXBxCTRL
  buff[1] = lowByte(id>>3);  // TXBxSIDH
  buff[2] = lowByte(id)<<5;// TXBxSIDL
  buff[3] = 0x00;
  buff[4] = 0x00;
  buff[5] = 0x02; // TXBxDLC
  buff[6]++;      // TXBxD0
  buff[7] = buff[6]+1;// TXBxD1
  CAN.write(REG_TXB0, buff, 8);
  id++;
  if(id == 2048) { id = 0; }
  //delay(10);
  //PrintRegister(REG_TXB0, "TXB0");
  Serial.println(id, HEX);
  delay(10);
  CAN.sendCommand(CMD_RTS0);  
  //delay(10);
  //CAN.PrintRegister(REG_CNF1, "CNF1");
  //CAN.PrintRegister(REG_CNF2, "CNF2");
  //CAN.PrintRegister(REG_CNF3, "CNF3");
}



