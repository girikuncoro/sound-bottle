const byte LSM303D_address = 0b011110;
const byte MULTI_READ = 0b01000000;
const byte READ = 0b10000000;
const byte MAG_REFRESH_100Hz = 0b00010100;
const byte TEMP_ON = 0b10000000;
const byte ACC_ALL_AXIS = 0b00000111;
const byte ACC_REFRESH_100Hz = 0b01100000;
const byte MAG_CONTINUOUS = 0b00000000;
const byte CTRL1 = 0x20;
const byte CTRL5 = 0x24;
const byte CTRL7 = 0x26;
const byte OUT_X_L_A = 0x28;
const byte OUT_Y_L_A = 0x2A;
const byte OUT_Z_L_A = 0x2C;
const byte TEMP_OUT_L = 0x05;

const int CS_pin = D0;
const int pt_pin = A0;
const int shake_pin = D1;
const int open_pin = D2;
const int close_pin = D3;

float maxs = 0.99;
float mins = -0.99;
bool bottle_open = false;

void setup() {
  Serial.begin(9600);                       // open the serial port

  pinMode(CS_pin,OUTPUT);
  pinMode(pt_pin,INPUT);
  pinMode(shake_pin,OUTPUT);
  pinMode(open_pin,OUTPUT);
  pinMode(close_pin,OUTPUT);

  digitalWrite(CS_pin,HIGH);                // init section
  digitalWrite(shake_pin,HIGH);
  digitalWrite(open_pin,HIGH);
  digitalWrite(close_pin,HIGH);

  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV32);
  SPI.begin();

  digitalWrite(CS_pin,LOW);                 // write section
  SPI.transfer(CTRL1);
  SPI.transfer(ACC_REFRESH_100Hz | ACC_ALL_AXIS);
  digitalWrite(CS_pin, HIGH);
  digitalWrite(CS_pin, LOW);
  SPI.transfer(CTRL5);
  SPI.transfer(MAG_REFRESH_100Hz | TEMP_ON);
  digitalWrite(CS_pin, HIGH);
  digitalWrite(CS_pin, LOW);
  SPI.transfer(CTRL7);
  SPI.transfer(MAG_CONTINUOUS);
  digitalWrite(CS_pin, HIGH);
}

void loop() {
  int16_t x_acc;
  int16_t y_acc;
  int16_t z_acc;

  digitalWrite(CS_pin,LOW);
  SPI.transfer(READ | OUT_X_L_A | MULTI_READ);
  x_acc = SPI.transfer(0);
  x_acc |= SPI.transfer(0) << 8 ;
  SPI.transfer(READ | OUT_Y_L_A | MULTI_READ);
  y_acc = SPI.transfer(0);
  y_acc |= SPI.transfer(0) << 8 ;
  SPI.transfer(READ | OUT_Z_L_A | MULTI_READ);
  z_acc = SPI.transfer(0);
  z_acc |= SPI.transfer(0) << 8 ;
  digitalWrite(CS_pin,HIGH);

  float x = (1.0*x_acc)/0x7FFF;
  float y = (1.0*y_acc)/0x7FFF;
  float z = (1.0*z_acc)/0x7FFF;

  /*Serial.print("X = ");
  Serial.print(x);
  Serial.print(" Y = ");
  Serial.print(y);
  Serial.print(" Z = ");
  Serial.print(z);
  Serial.println(" ");
  delay(100);*/

  bool shaking = x > maxs || x < mins;

  if (shaking) {
    Serial.print("SHAKING");
    digitalWrite(shake_pin, LOW);
    delay(300);
    digitalWrite(shake_pin, HIGH);
  }

  if (digitalRead(pt_pin) == 1) {             // bottle is open
    if (bottle_open == false) {
      bottle_open = true;
      digitalWrite(open_pin, LOW);
      delay(300);
      digitalWrite(open_pin, HIGH);
    }
  }

  if (digitalRead(pt_pin) == 0) {             // bottle is closed
    if (bottle_open == true) {
      bottle_open = false;
      digitalWrite(close_pin, LOW);
      delay(300);
      digitalWrite(close_pin, HIGH);
    }
  }
}
