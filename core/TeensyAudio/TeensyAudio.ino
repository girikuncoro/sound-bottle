//
// SOUND BOTTLE PROJECT
// Two wires must be connected to Photon for shake and open/close sensing.
//   Open/Close signal: pin 0 to Photon
//   Shake signal:      pin 1 to Photon   
//   
//   Interaction:
//     - Close first time: do nothing as default
//     - Open first time: start recording with sound detection
//     - Close second time, then open second time: start playing
//     - Shaking when open: stop and clear records
//     - Shaking when close: change pattern
//
//  NoClose: HIGH, Close: LOW
//  NoOpen: HIGH, Open: LOW
//  NoShake: HIGH, Shake: LOW
//

#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>


AudioInputI2S            i2s2;           //xy=105,63
AudioAnalyzePeak         peak1;          //xy=278,108
AudioRecordQueue         queue1;         //xy=281,63
AudioOutputI2S           i2s1;           //xy=470,120
AudioConnection          patchCord1(i2s2, 0, queue1, 0);
AudioConnection          patchCord2(i2s2, 0, peak1, 0);

AudioControlSGTL5000     sgtl5000_1;     //xy=265,212

AudioPlaySdRaw           playRaw1;       //xy=302,157
AudioPlaySdRaw           playRaw2;     //xy=204,291
AudioPlaySdRaw           playRaw3;
AudioPlaySdWav           playSdWav1;     //xy=211,320
AudioPlaySdWav           playSdWav2;     //xy=230,370
AudioPlaySdWav           playSdWav3;     //xy=165,290
AudioPlaySdWav           playSdWav4;     //xy=165,290
AudioMixer4              mixer1;         //xy=472,248
AudioMixer4              mixer2;         //xy=474,340
AudioConnection          patchCord11(playSdWav1, 0, mixer1, 2);
AudioConnection          patchCord12(playSdWav1, 1, mixer2, 2);
AudioConnection          patchCord3(playRaw2, 0, mixer1, 1);
AudioConnection          patchCord4(playRaw2, 0, mixer2, 1);
AudioConnection          patchCord5(playRaw1, 0, mixer1, 0);
AudioConnection          patchCord6(playRaw1, 0, mixer2, 0);
AudioConnection          patchCord7(playRaw3, 0, mixer1, 3);
AudioConnection          patchCord8(playRaw3, 1, mixer2, 3);
AudioConnection          patchCord9(mixer1, 0, i2s1, 0);
AudioConnection          patchCord10(mixer2, 0, i2s1, 1);


// Bounce objects to easily and reliably read the buttons
//Bounce buttonRecord = Bounce(0, 8);
Bounce signalClose =   Bounce(1, 8);  // 8 = 8 ms debounce time
Bounce signalOpen =   Bounce(2, 8);
Bounce signalShake = Bounce(3, 8);

bool isClosed = true;
bool isShaken = false;
bool waitToOpen = true;  // wait for the first open
bool recordReady = false;

// which input on the audio shield will be used?
const int myInput = AUDIO_INPUT_MIC;

// Remember which mode we're doing
const int STOP = 0;
const int RECORD = 1;
const int PLAY = 2;
int mode = STOP;

// Constants for recording
File frec;  // the file where data is recorded
int countFile = 0;  // count total files
const int SOUND_TO_RECORD = 3;
long startRecordTime, elapsedRecordTime;
int RECORD_TIME = 1000;  // 1 sec for each record

// Constants for playing
int countBeat = 1;
long beatStart, beatElapsed;
int BPM = 100;  // 100 msec :: 146 BPM
bool loopStop = false;

// Music pattern
int pattern = 1;
const int LAST_PATTERN = 2;

// Constants for sound detection from the red mic
const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;
const float SOUND_THR = 3.0;  // threshold for sound detection


void setup() {
  // Configure the signal pins
//  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  // Audio connections require memory, and the record queue
  // uses this memory to buffer incoming audio.
  AudioMemory(60);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(0.5);

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

  mixer1.gain(0, 0.5);
  mixer1.gain(1, 0.5);
  mixer2.gain(0, 0.5);
  mixer2.gain(1, 0.5);

  // wait for Photon to be ready
  delay(1000);
}


void loop() {
  // first, read the signals
  signalClose.update();
  signalOpen.update();
  signalShake.update();

  // wait till the first open
  while (waitToOpen) {
    if (signalOpen.fallingEdge()) {
      Serial.println("Bottle is opened for the first time");
      waitToOpen = false;
      recordReady = true;
      isClosed = false;
    }
    signalOpen.update();
  }

  // ready to record
  if (mode == STOP && recordReady && !isClosed) detectSound();

  // shake to change pattern
  if (signalShake.fallingEdge()) {
    Serial.println("Bottle is shaken");
    if (isClosed) {
      // change pattern
      if (pattern == LAST_PATTERN) {
        pattern = 1;
      } else {
        pattern++;
      }
      Serial.print("Shake pattern now : "); Serial.println(pattern);
    } else {
      // reset bottle state
      Serial.println("Bottle reset, next open will be record");
      stopPlaying();
      recordReady = true;
      waitToOpen = true;
    }
  }

  if (signalClose.fallingEdge()) {
    Serial.println("Bottle is closed");
    loopStop = true;
    isClosed = true;
    
    if (mode == RECORD) stopRecording();
    if (mode == PLAY) stopPlaying();
  }
  
  if (signalOpen.fallingEdge()) {
    Serial.println("Bottle is opened");
    isClosed = false;
   
    if (mode == STOP) startPlaying();
    loopStop = false;
    beatStart = millis();  // start the beat metronome
  }

  // if we're recording or playing, carry on...
  if (mode == RECORD) continueRecording();
  if (mode == PLAY) continuePlaying();

  // when using a microphone, adjust gain
  if (myInput == AUDIO_INPUT_MIC) adjustMicLevel();
}

void startRecording() {
  Serial.println("startRecording");
  Serial.print("File number: "); Serial.println(countFile, DEC);
  
  String tmp = "RECORD";
  tmp.concat(countFile);
  tmp.concat(".RAW");
  char fileName[tmp.length()+1];
  tmp.toCharArray(fileName, sizeof(fileName));

  Serial.print("File name: "); Serial.println(fileName);

  if (SD.exists(fileName)) {
    SD.remove(fileName);
  }
  frec = SD.open(fileName, FILE_WRITE);
  
  if (frec) {
    queue1.begin();
    mode = RECORD;
  }

  if (countFile == SOUND_TO_RECORD - 1) {
    countFile = 0;
  } else {
    countFile++;
  }
  
  startRecordTime = millis();
}

void continueRecording() {
  if (queue1.available() >= 2) {
    byte buffer[512];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer+256, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    // write all 512 bytes to the SD card
    frec.write(buffer, 512);
  }
  elapsedRecordTime = millis() - startRecordTime;
  if (elapsedRecordTime > RECORD_TIME) stopRecording();
}

void stopRecording() {
  Serial.println("stopRecording");
  queue1.end();
  if (mode == 1) {
    while (queue1.available() > 0) {
      frec.write((byte*)queue1.readBuffer(), 256);
      queue1.freeBuffer();
    }
    frec.close();
  }
  mode = STOP;
}


void startPlaying() {
  Serial.print("Count beat: "); Serial.println(countBeat, DEC);
//  if (pattern == 1) {
//    playSdWav1.play("KRNE1.wav");
//    playSdWav3.play("KRNE6.wav");
//    playSdWav4.play("KRNE8.wav");
//  } else if (pattern == 2) {
//    playSdWav1.play("KRNE1.wav");
//    playSdWav3.play("KRNE6.wav");
//  }
  if (pattern == 1) {
    playRaw1.play("RECORD0.RAW");
    playSdWav1.play("KRNE6.wav");
  } else if (pattern == 2) {
    playRaw1.play("RECORD0.RAW");
    playRaw3.play("RECORD2.RAW");
  }
  mode = PLAY;
}

void continuePlaying() {
  beatElapsed = millis() - beatStart;
//  if (!playRaw1.isPlaying()) {
//    playRaw1.stop();
//    mode = 0;
//  }

  if (beatElapsed > BPM) {
    countBeat++;
    Serial.print("Count beat: "); Serial.println(countBeat, DEC);

    if (!playSdWav1.isPlaying()) {
      playSdWav1.stop();
    }
    if (!playRaw1.isPlaying()) {
      playRaw1.stop();
    }
    if (!playRaw2.isPlaying()) {
      playRaw2.stop();
    }
    if (!playRaw3.isPlaying()) {
      playRaw3.stop();
    }

    if (pattern == 1) {
      beat0();
    } else if (pattern == 2) {
      beat3();
    }

    if (countBeat == 32) {
      countBeat = 0;
    }
    
    beatStart = millis();
  }
}

void stopPlaying() {
  Serial.println("stopPlaying");
  if (mode == PLAY) {
    playRaw1.stop();
    playRaw2.stop();
    playRaw3.stop();
    playSdWav1.stop();
  }
  mode = STOP;
}

void adjustMicLevel() {
  sgtl5000_1.micGain(20);
}

void beat0() {
    if (countBeat == 1 || countBeat == 7 || countBeat == 8 || countBeat == 13 || countBeat == 17 || countBeat == 23 || countBeat == 24) {
      playRaw1.play("RECORD0.RAW");
      delay(30);
    }
    
    if (countBeat == 9 || countBeat == 25) {
      playRaw2.play("RECORD1.RAW");
      delay(30);
    }

    if (countBeat == 1) {
      playSdWav1.play("KRNE6.wav");
      delay(30);
    }
}

void beat3() {
  if (countBeat == 1 || countBeat == 8 || countBeat == 11 || countBeat == 14 || countBeat == 17 || countBeat == 21 || countBeat == 25 || countBeat == 28) {
    playRaw1.play("RECORD0.RAW");
    delay(30);
  }

  if (countBeat == 3 || countBeat == 5 || countBeat == 9 || countBeat == 12 || countBeat == 15 || countBeat == 16 || countBeat == 19 || countBeat == 23 || countBeat == 27) {
    playRaw2.play("RECORD1.RAW");
    delay(30);
  }

  if (countBeat == 1 || countBeat == 7 || countBeat == 12 || countBeat == 17 || countBeat == 21 || countBeat == 28) {
    playRaw3.play("RECORD2.RAW");
    delay(30);
  }
}

void beat1() {
    if (countBeat == 1 || countBeat == 7 || countBeat == 13 || countBeat == 17 || countBeat == 23) {
      playSdWav1.play("KRNE1.wav");
      delay(10);
    }

    if (countBeat == 8 || countBeat == 24) {
      playSdWav1.play("KRNE1.wav");
      delay(10);
    }
    
    if (countBeat == 9 || countBeat == 25) {
      playSdWav2.play("KRNE2.wav");
      delay(10);
    }

    if (countBeat == 1) {
      playSdWav3.play("KRNE6.wav");
      delay(10);
    }

    if (countBeat == 1 || countBeat == 4) {
      playSdWav4.play("KRNE8.wav");
      delay(10);
    }
}

void beat2() {
  if (countBeat == 1 || countBeat == 4 || countBeat == 7 || countBeat == 19 || countBeat == 21 || countBeat == 32) {
    playSdWav1.play("KRNE1.wav");
    delay(10);
  }

  if (countBeat == 9 || countBeat == 25) {
    playSdWav2.play("KRNE2.wav");
    delay(10);
  }

  if (countBeat == 1) {
     playSdWav3.play("KRNE6.wav");
     delay(10);
   }

//  if (countBeat == 1) {
//    playSdWav4.play("KRNE8.wav");
//    delay(10);
//  }
}

void detectSound() {
   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level
 
   unsigned int signalMax = 0;
   unsigned int signalMin = 1024;
 
   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(16);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
   double volts = (peakToPeak * 3.3) / 1024;  // convert to volts

  // start recording when sound reach threshold
  if (volts > SOUND_THR) {
    startRecording();
  }
}

