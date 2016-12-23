void setup() {
  // put your setup code here, to run once:
Serial.begin(115200);
Serial.println();
Serial.println();
Serial.print("Flash Size");
Serial.println(ESP.getFlashChipSize());
}

void loop() {
  // put your main code here, to run repeatedly:
yield();
}
