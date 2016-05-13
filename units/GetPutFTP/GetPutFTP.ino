// Test FTP Put and Get command
// Configuration:
//   - load FTP server in MAC, registered the ip address on Wifly
//   - Baudrate 19200 for Serial1, open: @, close: #
//   - type 'ftp put <file-name>' in Serial monitor to upload files
//   - type 'ftp get <file-name>' in Serial monitor to get files
//   - change the file name on SD card fn variable
//   - Teensy connects (2) for RX and (3) for TX
//   - Wifly connects (2) for TX and (3) for RX
//
// TODO: Implement flow control to increase baudrate

#include <Arduino.h> 
#include <SD.h>
#include <SPI.h>

int led = 13; // The LED pin on Teensy 3.1

// Used to buffer Serial stuff
String content = "";
char character;

String content3 = "";
char character3;

File myFile;

/************************************************************************/
// Useful to show that at least something is happening in case no text output appears.
// E.g. if you wonder if you fried the board or something.
void blink() {
  delay(100); 
  digitalWrite(led, HIGH);
  delay(100);
  digitalWrite(led, LOW);
  delay(100);
}

/************************************************************************/
void setup() {
  pinMode(led, OUTPUT); // THIS IS IMPORTANT, or else you never see the light 
  Serial1.begin(19200);  // UART, RX (D0), TX (D1) are connected to WiFly
  Serial.begin(19200);   // Be sure to set USB serial in the IDE.

  blink();  

  Serial.println("Are we ready?");
  blink();
  
  // Initialize the SD card
  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (!(SD.begin(10))) {
    // stop here if no SD card, but print a message
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  Serial.println("Card is successfully initialized");
}

/************************************************************************/
void loop() {

  String content = "";
  char character;

  String content3 = "";
  char character3;
  
  char charBuff[512];

  while(Serial1.available() > 0) {
    int len = Serial1.readBytes(charBuff, 512);

    for(int i = 0; i < len; i++) {
         Serial.print(charBuff[i]);
    }
      
    if(myFile) {
      myFile.write(charBuff, len);
    } else {
      Serial.println("error opening audio_speed.RAW");
    }
    memset(charBuff, 0, sizeof(charBuff));
  }

  if (content != "") {
    Serial.print(content);
  }

  while(Serial.available()) {
    character3 = Serial.read();
    content3.concat(character3);
  }

  if(content3 == "$$$") {
    Serial1.write("$$$");
  } else if (content3 == "close") {
    Serial.println("Closing the SD card");
    myFile.close();
  } else if (content3 == "#") {
    Serial.println("closing ftp...");
    Serial1.write("#");
  } else if (content3 != "") {
    String tmp = content3.substring(0,7);
    if (tmp == "ftp put") {
      Serial.println("Putting file to FTP.. ");
      Serial1.println(content3);

      // waiting for "FTP connecting" ack from server
      while(!Serial1.available());
      int len = Serial1.readBytes(charBuff, 512);

      for(int i = 0; i < len; i++) {
         Serial.print(charBuff[i]);
      }

      openFile("audio.raw");
      putFTP();
    } else {
      Serial.println(content3);
      Serial1.println(content3);
      delay(3000);
    }
  }

  blink();   
}

/************************************************************************/
void openFile(char fn[]) {
  Serial.print("Try to open "); Serial.println(fn);
  
  myFile = SD.open(fn, FILE_READ);
  if (!myFile) {
    Serial.println("Failed to open file");
    while(1);
  }
}

/************************************************************************/
void putFTP() {
  byte sendBuf[512];
  int count = 0;

  while(myFile.available()) {
    int len = myFile.readBytes(sendBuf, 512);
    Serial1.write(sendBuf, len);
    memset(sendBuf, 0, sizeof(sendBuf));
  }

  Serial.println(" "); Serial.println("Putting file is done...");
  Serial.println("Waiting to timeout");
  while(!Serial1.available()) {};
  Serial.println("FTP Done..");
  myFile.close();
}

