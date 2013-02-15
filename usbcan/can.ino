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

CAN::CAN(byte pin)
{
  // set the Slave Select Pin as an output:
  ss_pin = pin;
  pinMode (ss_pin, OUTPUT);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  // initialize SPI:
  SPI.begin();
  digitalWrite(ss_pin, HIGH);
}

void CAN::write(byte reg, byte *buff, byte count)
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

void CAN::read(byte reg, byte *buff, byte count)
{
  byte n;
  digitalWrite(ss_pin, LOW);
  SPI.transfer(CMD_READ);
  SPI.transfer(reg);
  for(n = 0; n < count; n++) {
    buff[n] = SPI.transfer(0x00);
  }  
  digitalWrite(ss_pin, HIGH);
}

void CAN::sendCommand(byte command)
{
  digitalWrite(ss_pin, LOW);
  SPI.transfer(command);
  digitalWrite(ss_pin, HIGH);
}

void CAN::setBitRate(int bitrate)
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

void CAN::setMode(byte mode)
{
  byte buff;
  buff = mode;
  write(REG_CANCTRL, &buff, 1);
}

byte CAN::getMode(void)
{
  byte buff;
  read(REG_CANSTAT, &buff, 1);
  return buff >> 5;
}

byte CAN::getRxStatus(void)
{
  byte result;
  digitalWrite(ss_pin, LOW);
  SPI.transfer(CMD_RXSTATUS);
  result = SPI.transfer(0x00);
  digitalWrite(ss_pin, HIGH);
  return result;
}

CanFrame CAN::readFrame(byte rxb)
{
  byte sidl, sidh, eid0, eid8, n;
  CanFrame frame;
  digitalWrite(ss_pin, LOW);
  SPI.transfer(CMD_READRXB | (rxb<<2));
  sidh = SPI.transfer(0x00); //SIDH
  sidl = SPI.transfer(0x00); //SIDL
  eid8 = SPI.transfer(0x00); //EID8
  eid0 = SPI.transfer(0x00); //EID0
  frame.length = SPI.transfer(0x00);  //DLC
  for(n=0; n<frame.length; n++) {
    frame.data[n] = SPI.transfer(0x00);
  }
  digitalWrite(ss_pin, HIGH);
  if(sidl & 0x08) {   // Extended Identifier
    sidl &= 0xE3;     // Clear the EXIDE bit
    frame.id = ((unsigned long)sidh << 21);
    frame.id |= ((unsigned long)sidl & 0xE0)<<13;
    frame.id |= ((unsigned long)sidl & 0x03)<<16;
    frame.id |= ((unsigned long)eid8<<8) | (unsigned long)eid0;
    frame.eid = 0x01;
  } else {
    frame.id = (sidh<<3) | (sidl>>5);
    frame.eid = 0x00;
  }
  return frame;
}

byte CAN::writeFrame(CanFrame frame)
{
  byte result, i, j;
  byte eid0, eid8, sidl, sidh;
  byte ctrl[] = {REG_TXB0CTRL, REG_TXB1CTRL, REG_TXB2CTRL};

  for(i=0; i<3; i++) {  //Loop through all three TX buffers
     read(ctrl[i], &result, 1); //Read the TXBxCTRL register
     if(!(result & TXREQ)) {    //If the TXREQ bit is not set then...
       if(frame.eid) {
         eid0 = frame.id;        //Assemble the buffers
         eid8 = frame.id >>8;
         sidl = ((frame.id>>16) & 0x03) | ((frame.id>>13) & 0xE0) | 0x08;
         sidh = frame.id>>21;
       } else {
         sidl = frame.id<<5;
         sidh = frame.id>>3;
       }
         
       digitalWrite(ss_pin, LOW);
       SPI.transfer(CMD_WRITETXB | (i<<1));   //Write the buffers
       SPI.transfer(sidh); //SIDH
       SPI.transfer(sidl); //SIDL
       SPI.transfer(eid8); //EID8
       SPI.transfer(eid0); //EID0
       SPI.transfer(frame.length);  //DLC
       for(j=0; j<frame.length; j++) {
          SPI.transfer(frame.data[j]);
       }
       digitalWrite(ss_pin, HIGH);  // Might need delay here
       sendCommand(CMD_RTS | (0x01 << i));
       return 0;
     }
  }   
  return -1;  //All the buffers are full
}

