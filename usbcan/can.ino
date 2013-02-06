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
#include "can.h"

void _CAN::begin(byte pin)
{
  // set the Slave Select Pin as an output:
  ss_pin = pin;
  pinMode (ss_pin, OUTPUT);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  // initialize SPI:
  SPI.begin();
  digitalWrite(ss_pin, HIGH);
}

void _CAN::write(byte reg, byte *buff, byte count)
{
  byte n;
  digitalWrite(ss_pin, LOW);
  SPI.transfer(CMD_WRITE);
  SPI.transfer(reg);
  for(n = 0; n < count; n++) {
    SPI.transfer(buff[n]);
  }  
  digitalWrite(ss_pin, HIGH);
}

void _CAN::sendCommand(byte command)
{
  digitalWrite(ss_pin, LOW);
  SPI.transfer(command);
  digitalWrite(ss_pin, HIGH);
}

void _CAN::setBitRate(int bitrate)
{
  byte buff[3];
  switch(bitrate) {
    case 125:
      buff[0] = CNF3_125;
      buff[1] = CNF2_125;
      buff[2] = CNF1_125;
      break;
    case 250:
      buff[0] = CNF3_250;
      buff[1] = CNF2_250;
      buff[2] = CNF1_250;
      break;
    case 500:
      buff[0] = CNF3_500;
      buff[1] = CNF2_500;
      buff[2] = CNF1_500;
      break;
    case 1000:
      buff[0] = CNF3_1000;
      buff[1] = CNF2_1000;
      buff[2] = CNF1_1000;
      break;
    default:
      return;
  }
  write(REG_CNF3, buff, 3);
}

void _CAN::setMode(byte mode)
{
  byte buff;
  buff = mode;
  write(REG_CANCTRL, &buff, 1);
}

/* This function is for testing only */
void _CAN::PrintRegister(byte reg, char *str)
{
  byte bb;
  digitalWrite(ss_pin, LOW);
  SPI.transfer(CMD_READ); // Read
  SPI.transfer(reg); 
  bb = SPI.transfer(0x00);
  digitalWrite(ss_pin, HIGH);
  Serial.print(str);  
  Serial.print(" = ");
  Serial.println(bb, HEX);
} 
