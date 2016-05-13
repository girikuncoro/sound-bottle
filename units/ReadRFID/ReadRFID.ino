// RFID tag detection, send flag if RFID tag is not ours
// Each bottle will have both RFID reader and tag, so we need this
// Config:
//   - RFID (4) to Photon (D0) with pull-up 10K, which is IRQ
//   - RFID (5) to Photon (D1) with pull-up 10K, which is RST
//   - RFID (2) to Photon (D2), which is EXT Interrupt1
//   - RFID (3) to Photon (D3), which is EXT Interrupt2
//   - Power RFID with 5V and GND

#include "Adafruit_PN532.h"

// I2C Mode defines
#define IRQ_PIN  (D2) // This is how the PN532 Shield gets ready status in I2C mode
#define RST_PIN  (D3) // Necessary for I2C mode

Adafruit_PN532 nfc(IRQ_PIN, RST_PIN);

// This bottle's RFID UID, change this for every bottle
uint8_t this_uid[] = { 12, 130, 59, 10, 0, 0, 0 };

void setup(void) {
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

  Serial.println("Waiting for an ISO14443A Card ...");
}


void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    Serial.println("Raw UID");
    for (int i = 0; i < 7; i++) {
      Serial.print(uid[i]); Serial.print(" ");
    }
    Serial.println("");

    if (!isOurUID(uid, this_uid)) {
      Serial.println("This is not our UID");

      // TODO: pouring interaction, should be combined with gyro to trigger

    } else {
      Serial.println("This IS our UID");
    }

    // wait for another card
    delay(1000);
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
