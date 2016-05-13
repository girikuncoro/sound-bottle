#include "Adafruit_PN532.h"

// I2C Mode defines
#define IRQ_PIN  (D2) // This is how the PN532 Shield gets ready status in I2C mode
#define RST_PIN  (D3) // Necessary for I2C mode

Adafruit_PN532 nfc(IRQ_PIN, RST_PIN);

uint8_t this_uid[] = { 50, 232, 176, 69, 0, 0, 0 };
uint8_t bottle1_uid[] = { 50, 232, 176, 69, 0, 0, 0 };
uint8_t bottle2_uid[] = { 226, 28, 172, 69, 0, 0, 0 };
uint8_t bottle3_uid[] = { 194, 25, 172, 69, 0, 0, 0 };

const int WAIT_BOTTLE = 0;
const int BOTTLE1 = 1;
const int BOTTLE2 = 2;
const int BOTTLE3 = 3;

int pairBottle = WAIT_BOTTLE;

// TODO: this should come from sensor
bool pouring = true;

void setup() {
  Serial.begin(9600);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();
  // TODO: dummy for debugging
  /*Particle.subscribe("bottle1-pour", handler);*/

  Serial.println("Waiting for an ISO14443A Card ...");
}

void loop() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Display some basic information about the card
    /*Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    Serial.println("Raw UID");
    for (int i = 0; i < 7; i++) {
      Serial.print(uid[i]); Serial.print(" ");
    }
    Serial.println("");*/

    if (!isOurUID(uid, this_uid)) {
      Serial.println("This is not our UID");
      pairBottle = getBottleID(uid);

      Particle.unsubscribe();
      if (pairBottle == BOTTLE1) {
        Serial.println("Subscribe to bottle1");
        Particle.subscribe("bottle1-pour", handler);
      } else if (pairBottle == BOTTLE2) {
        Serial.println("Subscribe to bottle2");
        Particle.subscribe("bottle2-pour", handler);
      } else if (pairBottle == BOTTLE3) {
        Serial.println("Subscribe to bottle3");
        Particle.subscribe("bottle3-pour", handler);
      }
    }

    if (pouring) {
      Serial.println("Should publish, this bottle is pouring");
      Particle.publish("bottle1-pour");  // TODO: change this accordingly
      pouring = false;  // should only detect once
    }

    // wait for another card
    delay(500);
  }
}

bool isOurUID(uint8_t uid1[], uint8_t uid2[]) {
  for (int i = 0; i < 7; i++) {
    if (uid1[i] != uid2[i]) {
      return false;
    }
  }
  return true;
}

int getBottleID(uint8_t uid[]) {
  for (int i = 0; i < 7; i++) {
    if (bottle1_uid[i] != uid[i]) break;
    if (i == 7) return BOTTLE1;
  }

  for (int i = 0; i < 7; i++) {
    if (bottle2_uid[i] != uid[i]) break;
    if (i == 7) return BOTTLE2;
  }

  for (int i = 0; i < 7; i++) {
    if (bottle3_uid[i] != uid[i]) break;
    if (i == 7) return BOTTLE3;
  }

  return -1;
}

void handler(const char *event, const char *data) {
  Serial.println("Pouring is detected");
  // TODO: should send get trigger to Teensy
}
