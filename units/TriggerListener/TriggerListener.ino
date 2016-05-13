// This photon has eventName "bottle2-pour" and listen to "bottle1-pour"
// Change the parameter on Particle.subscribe and Particle.publish

void setup() {
  Serial.begin(9600);
  Particle.subscribe("bottle2-pour", handler);
}

void loop() {
  char data = Serial.read();
  if (data == 'l') {
    Serial.println("Should publish");
    Particle.publish("bottle1-pour");
  }
}

void handler(const char *event, const char *data) {
  Serial.println("Pouring is detected from bottle 2");
}
