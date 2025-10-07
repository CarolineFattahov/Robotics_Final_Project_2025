#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
 
#define SS_PIN 10 // SPI SS for MFRC522
#define RST_PIN 4 // Reset for MFRC522

// Pins association
constexpr int RED_PIN = 5; 
constexpr int GREEN_PIN = 6;
constexpr int BLUE_PIN = 3;
constexpr int SERVO_PIN = 8;  
///////////////////

// Servo write in different situations
constexpr int SERVO_OPEN = 0; 
constexpr int SERVO_CLOSE = 90; 
//////////////////////////////////////

constexpr unsigned long RFID_SCAN_MS = 10000; // 10 sec
constexpr const char* AUTHORIZED_UID= "E2 0E 66 EE"; // Authorized UID

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
 
Servo doorServo;
String command;

bool lightEnabled = false; //Initialize led mode to be off

// Colors 
struct Color {int r, g, b;};
struct ColorMap {const char* name; Color c; };

constexpr ColorMap COLOR_MAP[] = {
    {"purple", {170,   0, 255}},
    {"green",  {  0, 255,   0}},
    {"red",    {255,   0,   0}},
    {"blue",   {  0,   0, 255}},
    {"yellow", {255, 255,   0}},
    {"white",  {255, 255, 255}},
    {"orange", {255,  60,   0}}
};
///////////

void ack() {
  Serial.println(F("DONE"));
}

void setColor(int r, int g, int b){
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

bool setColorByName(const String& name) {
  for (auto &cc :COLOR_MAP){
    if (name.equalsIgnoreCase(cc.name)){
      setColor(cc.c.r, cc.c.g, cc.c.b);
      return true;
    }
  }
  return false;
}

// Light Functions
void lightOn(){
  lightEnabled=true;
  setColor(255,255,255);
}

void lightOff(){
  lightEnabled=false;
  setColor(0,0,0);
}
///////////////////

// UID and door (servo) functions 
bool readAuthorizedCardWithin(unsigned long window_ms) {
  const unsigned long start = millis();
  while (millis() - start < window_ms) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String content = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) {
          content.concat(" 0");
        } else {
          content.concat(" ");
        }
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
      }
      content.toUpperCase();
      // Clear
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();

      // Comparison to a predefined string
      if (content.substring(1) == AUTHORIZED_UID) {
        return true;
      } else {
        return false;
      }
    }
    yield();
  }
  return false; // UID not found at time allocated
}

void handleDoorOpen() {
  Serial.println(F("Waiting for card... (10 sec)"));
  if (!readAuthorizedCardWithin(RFID_SCAN_MS)) {
    Serial.println(F("No card detected or access denied"));
    return;
  }
  Serial.println(F("Authorized access, opening door"));
  doorServo.write(SERVO_OPEN);
}

void handleDoorClose() {
  doorServo.write(SERVO_CLOSE);
}
//////////////////////////////////

// Process commands 
void handleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  if (cmd == "light on")  { lightOn();  ack(); return; }
  if (cmd == "light off") { lightOff(); ack(); return; }
  if (cmd == "open door") { handleDoorOpen(); ack(); return; }
  if (cmd == "close door"){ handleDoorClose(); ack(); return; }
  if (!lightEnabled) {
    Serial.println(F("Light is off"));
    ack();
    return;
  }
  if (setColorByName(cmd)) { ack(); return; }
  Serial.println(F("Unknown command"));
  ack();
}
////////////////////

void setup() {
  Serial.begin(9600); // Initiate a serial communication

  // Door (servo) pins
  doorServo.attach(SERVO_PIN);
  doorServo.write(SERVO_CLOSE);

  // RGB pins and initialization
  pinMode(RED_PIN,  OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  setColor(0,0,0);

  // RFID
  SPI.begin();
  mfrc522.PCD_Init();

}

void loop() {
  if (Serial.available()) {
    command = Serial.readStringUntil('\n');
    handleCommand(command);
    }
}





