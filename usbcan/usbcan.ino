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
/* If the sheild has a 16Mhz Oscillator comment the
   the following line.  This must come before the
   inclusion of "can.h".  See can.h for details */
//#define CAN_MHZ 20
/* This can be changed if the slave select output
   is a different pin than 10 */
#define PIN_SS 10

#include "can.h"

#define RXBUFF_SIZE 32
#define TXBUFF_SIZE 32
#define CANBUFF_SIZE 32

CAN Can((byte)PIN_SS);
char rxbuff[RXBUFF_SIZE];
byte rxbuffidx;
char txbuff[TXBUFF_SIZE];
byte txbuffidx;
byte txbufflen;
CanFrame canbuff[CANBUFF_SIZE];
byte canidx;
word id;

byte buff[16];

//#define FULL_HELP 1
char *helpString[] = {"CAN-FIXit USB to CAN Interface Adapter\n",
                               "Version 1.0\n\n",
#ifdef FULL_HELP
                               "  Command            Code\n",
                               "-------------------------\n",
                               "  Reset               K\n",
                               "  Set Bitrate         B\n",
                               "  Send Frame          W\n",
                               "  Open Port           O\n",
                               "  Close Port          C\n",
                               "  Set Mask            M\n",
                               "  Set Filter          F\n",
                               "  MCP2515 Raw Message Z\n",
                               "  Help                H\n",
#endif 
                               NULL};

void sendString(char *str)
{
  int n;
  while(str[n] != 0) {
    Serial.write(str[n]);
    n++;
    //TODO: Check for new CAN frame at every character
  }
}

void sendByte(char ch)
{
  Serial.write(ch);
  //TODO: Check for new CAN frame at every character
}  

                               
inline void sendHelp(void)
{
  byte index = 0;
  while(helpString[index] != NULL) {
    sendString(helpString[index]);
    index++;
  }
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
    sendString("*2\n");
    return;
  }
  if(mode == MODE_NORMAL) {
    Can.setMode(MODE_NORMAL);
  }
  sendString("b\n");
}

/* Get's the ID from the string.  Returns 0x00 on
   success or the error character that should be
   sent after the '*' if failure */

byte getID(char *buff, CanFrame *frame)
{
  if(buff[3]==':' || buff[3]=='\n') { //Standard frame
    buff[3] = 0;
    frame->id = strtoul(&buff[0], NULL, 16);
    frame->eid = 0x00;
    if(frame->id > 0x7FF) return '4';
  } else if(buff[8]==':' || buff[8] == '\n') { //Extended frame
    buff[8] = 0;
    frame->id = strtoul(&buff[0], NULL, 16);
    frame->eid = 0x01;
    if(frame->id > 0x1FFFFFFFL) return '4';
  } else {
    return '1';
  }
  return 0x00; 
}

inline void writeFrame(void)
{
  CanFrame frame;
  char chh, chl;
  byte n;

  if(chh = getID(&rxbuff[1], &frame)) {
    sendString("*");
    sendByte(chh);
    sendString("\n");
    return;
  }
  if(Can.getMode() != MODE_NORMAL) {
    sendString("*6\n");
  } else {
    frame.length=0;
    n = 5 + (5 * frame.eid); //Set to the first data char in buffer
    while(rxbuff[n] != '\n') {
      if(rxbuff[n+1]=='\n') { // Not an even number of data characters
        sendString("*1\n");
        return;
      }
      if(frame.length == 8) {
        sendString("*1\n");
        return;
      }
      
      chh = toupper(rxbuff[n]);
      chl = toupper(rxbuff[n+1]);
      if( ((chh<'0' || chh>'9') && (chh<'A' || chh>'F')) ||
          ((chl<'0' || chl>'9') && (chl<'A' || chl>'F'))) {
            sendString("*1\n");
            return;
      }
      if(chh > '9') {//It's a character
          frame.data[frame.length] = (chh - 55)<<4;
      } else {
          frame.data[frame.length] = (chh - '0')<<4;
      }
      if(chl > '9') {
          frame.data[frame.length] |= chl - 55;
      } else {
          frame.data[frame.length] |= chl - '0';
      }
      frame.length++;
      n+=2;
    }
    if(Can.writeFrame(frame)) {
      sendString("*3\n");
    } else {  
      sendString("w\n");
    }
  }
}

inline void writeFilter(void)
{
  CanFrame frame;
  char ch;
  byte mode, reg;
  byte buff[4];
  
  if(ch = getID(&rxbuff[3], &frame)) {
    sendString("*");
    sendByte(ch);
    sendString("\n");
    return;
  }
  mode = Can.getMode();
  if(mode == MODE_NORMAL) {
    Can.setMode(MODE_CONFIG);
  }
  ch = rxbuff[1] - '0';
  if(ch > 5) {
    sendString("*1\n");
    return;
  }
  if(ch < 3) reg = ch << 2;
  else       reg = (ch - 3) <<2 | 0x10;
  Serial.println(reg, HEX);
  if(frame.eid) {
    buff[3] = frame.id;        //Assemble the buffers
    buff[2] = frame.id >>8;
    buff[1] = ((frame.id>>16) & 0x03) | ((frame.id>>13) & 0xE0) | 0x08;
    buff[0] = frame.id>>21;
    Can.write(reg, buff, 4);
  } else {
    buff[1] = frame.id<<5;
    buff[0] = frame.id>>3;
    Can.write(reg, buff, 2);
  }

  if(mode == MODE_NORMAL) {
    Can.setMode(MODE_NORMAL);
  }
  sendString("f\n");
}

inline void writeMask(void)
{
  CanFrame frame;
  char ch;
  byte mode, reg;
  if(ch = getID(&rxbuff[3], &frame)) {
    sendString("*");
    sendByte(ch);
    sendString("\n");
    return;
  } 
  mode = Can.getMode();
  if(mode == MODE_NORMAL) {
    Can.setMode(MODE_CONFIG);
  }
  if(rxbuff[1] == '0')      reg = REG_RXM0SIDH;
  else if(rxbuff[1] == '1') reg = REG_RXM1SIDH;
  else {
    sendString("*1\n");
    return;
  }
  if(frame.eid) {
    buff[3] = frame.id;        //Assemble the buffers
    buff[2] = frame.id >>8;
    buff[1] = ((frame.id>>16) & 0x03) | ((frame.id>>13) & 0xE0) | 0x08;
    buff[0] = frame.id>>21;
    Can.write(reg, buff, 4);
  } else {
    buff[1] = frame.id<<5;
    buff[0] = frame.id>>3;
    Can.write(reg, buff, 2);
  }

  
  if(mode == MODE_NORMAL) {
    Can.setMode(MODE_NORMAL);
  }
  sendString("m\n");
}

/* Called if a full sentence has been received.  Determines
   what to do with the message */
inline void cmdReceived(void)
{
  if(rxbuff[0]=='O' && rxbuff[1] == '\n') {
    Can.setMode(MODE_NORMAL);
    sendString("o\n");
  } else if(rxbuff[0]=='C' && rxbuff[1] == '\n') {
    Can.setMode(MODE_CONFIG);
    sendString("c\n");
  } else if(rxbuff[0]=='K' && rxbuff[1] == '\n') {
    Can.sendCommand(CMD_RESET);
    sendString("k\n");
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
    sendString("*1\n");
  }
}  

/* Prints a received frame to serial port in the proper format */
void printFrame(CanFrame frame)
{
  byte n;
  char str[9];
  sendString("r");
  if(frame.eid) sprintf(str, "%08lX", frame.id);
  else          sprintf(str, "%03X", frame.id); 
  sendString(str);
  sendString(":");
  for(n=0;n<frame.length;n++) {
    if(frame.data[n]<0x10) sendString("0");
    sprintf(str, "%02X", frame.data[n]);
    sendString(str);
    //Serial.print(frame.data[n], HEX);
  }
  sendString("\n");
}


void setup() {
  Serial.begin(115200);
  Can.sendCommand(CMD_RESET);
  Can.setBitRate(125);
}

void loop()
{
  byte result;
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
      if(rxbuffidx >= RXBUFF_SIZE) rxbuffidx = 0;
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



