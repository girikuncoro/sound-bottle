const int get_pin = TX;
const int put_pin = RX;
const int pt_pin = WKP;                       // phototransistor
const int oe_pin = DAC;                       // output enable (OE) for 8-bit shift reg
const int accel_pin = A2;                     // chip select (CS) for accel/gyro
const int shift_pin = A1;                     // CS for 8-bit shift register
const int sound_pin = A0;                     // get sound clip detected from teensy
const int shake_pin = D7;                     // send shake detection to teensy
const int open_pin = D4;                      // send open detection to teensy
const int close_pin = D5;                     // send close detection to teensy

const int16_t green_pattern[] = {0x01, 0x02, 0x04, 0x08, 0x0};
const int16_t blue_pattern[] = {0x10, 0x20, 0x40, 0x80, 0x0};
const int16_t combo_pattern[] = {0x11, 0x22, 0x44, 0x88, 0x0};
const int16_t long_pattern[] = {0x11, 0x33, 0x22, 0x66, 0x44, 0xCC, 0x88, 0x0};
const int16_t green[] = {0xF, 0xF, 0xF, 0xF};
const int16_t blue[] = {0xF0, 0xF0, 0xF0, 0xF0};
const int16_t combo[] = {0xFF, 0xFF, 0xFF, 0xFF};

const byte READ = 0b10000000;                 // registers for accel/gyro
const byte CTRL1_XL = 0x10;
const byte CTRL9_XL = 0x18;
const byte CTRL10_C = 0x19;
const byte CTRL2_G = 0x11;
const byte OUT_X_L_G = 0x22;
const byte OUT_Y_L_G = 0x24;
const byte OUT_Z_L_G = 0x26;
const byte OUT_X_L_XL = 0x28;
const byte OUT_Y_L_XL = 0x2A;
const byte OUT_Z_L_XL = 0x2C;

const float acc_threshold = 0.70;             // acceleration threshold for shaking
const float rot_threshold = 0.60;             // rotation threshold for pouring

int16_t x_acc;
int16_t y_acc;
int16_t z_acc;
int16_t x_rot;
int16_t y_rot;
int16_t z_rot;
float xa;
float ya;
float za;
float xr;
float yr;
float zr;
float total_acc;
float total_rot;

bool bottle_open = false;                     // keeps track of the bottle status

void setup() {
  Serial.begin(9600);

  pinMode(get_pin, OUTPUT);                   // setup pins
  pinMode(put_pin, OUTPUT);
  pinMode(pt_pin, INPUT);
  pinMode(oe_pin, OUTPUT);
  pinMode(accel_pin, OUTPUT);
  pinMode(shift_pin, OUTPUT);
  pinMode(sound_pin, INPUT_PULLUP);
  pinMode(shake_pin, OUTPUT);
  pinMode(open_pin, OUTPUT);
  pinMode(close_pin, OUTPUT);

  digitalWrite(get_pin, HIGH);                // init section
  digitalWrite(put_pin, HIGH);
  digitalWrite(oe_pin, LOW);
  digitalWrite(accel_pin, HIGH);
  digitalWrite(shift_pin, HIGH);
  digitalWrite(shake_pin, HIGH);
  digitalWrite(open_pin, HIGH);
  digitalWrite(close_pin, HIGH);

  SPI.setBitOrder(MSBFIRST);                  // set up SPI interface
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV32);
  SPI.begin();

  digitalWrite(accel_pin, LOW);               // acc X, Y, Z axes enabled
  SPI.transfer(CTRL9_XL);
  SPI.transfer(0x38);
  digitalWrite(accel_pin, HIGH);

  digitalWrite(accel_pin, LOW);               // acc = 416Hz (High-Performance mode)
  SPI.transfer(CTRL1_XL);
  SPI.transfer(0x60);
  digitalWrite(accel_pin, HIGH);

  digitalWrite(accel_pin, LOW);               // gyro X, Y, Z axes enabled
  SPI.transfer(CTRL10_C);
  SPI.transfer(0x38);
  digitalWrite(accel_pin, HIGH);

  digitalWrite(accel_pin, LOW);               // gyro = 416Hz (High-Performance mode)
  SPI.transfer(CTRL2_G);
  SPI.transfer(0x8C);
  digitalWrite(accel_pin, HIGH);

  digitalWrite(shift_pin, LOW);               // set all LEDs off to begin
  SPI.transfer(0b00000000);
  digitalWrite(shift_pin, HIGH);

  //Particle.subscribe("bottle1-pour", handler);
}

void loop() {
  detect_open();
  detect_close();
  detect_sound();
  detect_shake();
  detect_pour();
}

void alert_teensy(int pin) {
  digitalWrite(pin, LOW);
  delay(300);
  digitalWrite(pin, HIGH);
}

void flash_leds(const int16_t* led_pattern, int size, int times) {
  digitalWrite(oe_pin, HIGH);
  for (int j=0; j < times; j++) {
    for (int i=0; i < size; i++) {
      digitalWrite(shift_pin, LOW);           // prepare a pulse on Latch
      SPI.transfer(led_pattern[i]);           // transfer the value
      digitalWrite(shift_pin, HIGH);          // Bit0..Bit7 are updated
      delay(50);
    }
  }
  digitalWrite(oe_pin, LOW);
}

void detect_open() {
  if ((digitalRead(pt_pin) == 0) && (bottle_open == false)) {
    Serial.println("BOTTLE OPENED");
    alert_teensy(open_pin);
    bottle_open = true;
  }
}

void detect_close() {
  if ((digitalRead(pt_pin) == 1) && (bottle_open == true)) {
    Serial.println("BOTTLE CLOSED");
    alert_teensy(close_pin);
    bottle_open = false;
  }
}

void detect_sound() {
  if (digitalRead(sound_pin) == LOW) {
    Serial.println("SOUND DETECTED");
    flash_leds(long_pattern, 8, 1);
    flash_leds(combo_pattern, 4, 2);
    delay(1000);
  }
}

void detect_shake() {
  digitalWrite(accel_pin, LOW);
  SPI.transfer(READ | OUT_X_L_XL);
  x_acc = SPI.transfer(0);
  x_acc |= SPI.transfer(0) << 8;
  digitalWrite(accel_pin, HIGH);

  digitalWrite(accel_pin, LOW);
  SPI.transfer(READ | OUT_Y_L_XL);
  y_acc = SPI.transfer(0);
  y_acc |= SPI.transfer(0) << 8;
  digitalWrite(accel_pin, HIGH);

  digitalWrite(accel_pin, LOW);
  SPI.transfer(READ | OUT_Z_L_XL);
  z_acc = SPI.transfer(0);
  z_acc |= SPI.transfer(0) << 8;
  digitalWrite(accel_pin, HIGH);

  xa = (1.0 * x_acc)/0x7FFF;
  ya = (1.0 * x_acc)/0x7FFF;
  za = (1.0 * x_acc)/0x7FFF;

  /*Serial.print("X = ");
  Serial.print(xa);
  Serial.print(" Y = ");
  Serial.print(ya);
  Serial.print(" Z = ");
  Serial.println(za);*/

  total_acc = (xa * xa);

  if (total_acc > acc_threshold) {
    Serial.print("SHAKING, TOTAL ACC = ");
    Serial.println(total_acc);
    flash_leds(blue, 2, 1);
    alert_teensy(shake_pin);
  }
}

void detect_pour() {
  digitalWrite(accel_pin, LOW);
  SPI.transfer(READ | OUT_X_L_G);
  x_rot = SPI.transfer(0);
  x_rot |= SPI.transfer(0) << 8;
  digitalWrite(accel_pin, HIGH);

  digitalWrite(accel_pin, LOW);
  SPI.transfer(READ | OUT_Y_L_G);
  y_rot = SPI.transfer(0);
  y_rot |= SPI.transfer(0) << 8;
  digitalWrite(accel_pin, HIGH);

  digitalWrite(accel_pin, LOW);
  SPI.transfer(READ | OUT_Z_L_G);
  z_rot = SPI.transfer(0);
  z_rot |= SPI.transfer(0) << 8;
  digitalWrite(accel_pin, HIGH);

  xr = (5.0 * x_rot)/0x7FFF;
  yr = (5.0 * y_rot)/0x7FFF;
  zr = (5.0 * z_rot)/0x7FFF;

  /*Serial.print("XR = ");
  Serial.print(xr);
  Serial.print(" YR = ");
  Serial.print(yr);
  Serial.print(" ZR = ");
  Serial.println(zr);*/

  total_rot = (yr * yr) + (zr * zr);

  if (total_rot > rot_threshold) {
    Serial.print("POURING, TOTAL ROT = ");
    Serial.println(total_rot);
    //Particle.publish("bottle2-pour");
    alert_teensy(put_pin);
  }
}

/*void handler(const char *event, const char *data) {
  alert_teensy(get_pin);
}*/
