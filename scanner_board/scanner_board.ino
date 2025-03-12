#include <HardwareSerial.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>
#include <PNGdec.h>
#include "logo.h"
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

const char* ssid = ""; // replace with your WIFI name
const char* password = ""; // replace with your WIFI password
const char* mqtt_server = ""; // replace with your broker url
const char* mqtt_username = ""; // replace with your username
const char* mqtt_password = ""; // replace with your password
const int mqtt_port =8883;

static const char *root_ca PROGMEM = R"EOF(
//your certificate
)EOF";

#define MAX_IMAGE_WIDTH 320
#define RX_BOARD 16
#define TX_BOARD 17
#define IN_SERVO_PIN 21
#define OUT_SERVO_PIN 14
#define IR_SCANNER_PIN 35
#define IR_OUT_PIN 34
#define RELAY_PIN 4
#define LED_STATUS 25
#define LED_RED 32
#define LED_GREEN 33
#define BUZZER 26

#define A1_SERVO_PIN 0


PNG png;
TFT_eSPI tft = TFT_eSPI();
HardwareSerial mySerial(1);
Servo outServo, inServo , servo_A1;


String isOpen = "";
int inDoorCloseAngle = 15;
int inDoorOpenAngle = 180;
int outDoorCloseAngle = 0;
int outDoorOpenAngle = 180;

bool doorOutOpen = false;
bool doorInOpen = false;
unsigned long doorInStartTime = 0;
unsigned long doorOutStartTime = 0;
unsigned long doorOpenDelay = 5000;

bool scanner_active = false;
unsigned long scannerStartTime = 0;
unsigned long scannerDelay = 4500;

bool LedGreenOn = false;
bool LedRedOn = false;
unsigned long LedOpenDelay = 1000;
unsigned long LedGreenStartTime = 0;
unsigned long LedRedStartTime = 0;



WiFiClientSecure espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE 50


struct ParkingSlot {
  String slot;  
  String plate;  
};

ParkingSlot parkingSlots[] = {
  {"A1", ""},
  {"A2", ""},
  {"A3", ""},
  {"B1", ""},
  {"B2", ""},
  {"B3", ""}
};

 
void setup() {

  Serial.begin(115200);
  setupWiFi();
  setupMQTT();

  mySerial.begin(9600, SERIAL_8N1, RX_BOARD, TX_BOARD);
  pinMode(IR_SCANNER_PIN, INPUT);
  pinMode(IR_OUT_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  outServo.attach(OUT_SERVO_PIN, 1000, 2000);
  inServo.attach(IN_SERVO_PIN, 1000, 2000);
  outServo.write(outDoorCloseAngle);
  inServo.write(inDoorCloseAngle);  
  servo_A1.attach(A1_SERVO_PIN , 1000 , 2000);
  servo_A1.write(0);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_WHITE);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed!");
    return;
  }

  int16_t rc = png.openFLASH((uint8_t*)LOGO, sizeof(LOGO), pngDraw);
  if (rc == PNG_SUCCESS) {
    Serial.println("Successfully opened PNG file");
    tft.startWrite();
    uint32_t dt = millis();
    rc = png.decode(NULL, 0);
    Serial.printf("Decoding time: %d ms\n", millis() - dt);
  }
  ScannerOff();
}

void loop() {
  displaySlots();
  loopMQTT();
  handleScanner();
  handleDoorOut();
  updateServo();
  updateLED();


  //status ready
  digitalWrite(LED_STATUS, HIGH);

}

  

void handleDoorOut() {
  if (digitalRead(IR_OUT_PIN) == LOW) {
    Serial.println("Door out open");
    outServo.write(inDoorOpenAngle);
    doorOutStartTime = millis();
    doorOutOpen = true;
  }
}

//---------------------------------------------------------- DOOR -----------------------------------------------

void openInDoor() {
  //led green on
  scanner_active = false;
  ScannerOff();

  digitalWrite(LED_GREEN, HIGH);
  LedGreenStartTime = millis();
  LedGreenOn = true;
  tone(BUZZER, 2500, 1000);
  delay(100);
  inServo.write(inDoorOpenAngle);

  doorInStartTime = millis();
  doorInOpen = true;
  scanner_active = false;
  ScannerOff();
}

void wrongKey() {
    Serial.println("WRONG");
  scanner_active = false;
  ScannerOff();
  digitalWrite(LED_RED, HIGH);
  LedRedStartTime = millis();
  LedRedOn = true;

  tone(BUZZER, 2500, 100);  // ติ๊ดแรกยาวขึ้นเล็กน้อย
  delay(150);               // เว้นระยะ 150ms เพื่อให้ฟังชัดขึ้น
  noTone(BUZZER);

  delay(100);               // เว้นระยะก่อน "ติ๊ด" ที่สอง
  tone(BUZZER, 2500, 100);  // ติ๊ดที่สอง
  delay(150);
  noTone(BUZZER);

  ScannerOff();
  delay(10);
  ScannerOn();
  scanner_active = true;
  scannerStartTime = millis();
}


void updateServo() {
  if (doorOutOpen && millis() - doorOutStartTime >= doorOpenDelay) {
    outServo.write(outDoorCloseAngle);
    doorOutOpen = false;
    Serial.println("ประตูปิด");
  }
  if (doorInOpen && millis() - doorInStartTime >= doorOpenDelay) {
    inServo.write(inDoorCloseAngle);
    doorInOpen = false;
    Serial.println("ประตูปิด");
  }
}

void updateLED() {
  if (LedGreenOn && millis() - LedGreenStartTime >= LedOpenDelay) {
    // outServo.write(0);
    LedGreenOn = false;
    digitalWrite(LED_GREEN, LOW);
  }
  if (LedRedOn && millis() - LedRedStartTime >= LedOpenDelay) {
    LedRedOn = false;
    digitalWrite(LED_RED, LOW);
  }
}


void handleScanner() {
  if (digitalRead(IR_SCANNER_PIN) == LOW) {
    if (!scanner_active && !doorInOpen) {
      Serial.println("Scanner Activated");
      ScannerOn();
      scannerStartTime = millis();
      scanner_active = true;
    }
    if (mySerial.available()) {
      tone(BUZZER, 2500, 50);
      String data = mySerial.readString();
      data.trim();
      Serial.println("Scanned Data: " + data);

      publishJsonMQTT(data);
      scanner_active = false;

      ScannerOff();
    }
    if (scanner_active && millis() - scannerStartTime >= scannerDelay) {
      Serial.println("Scanner Timeout: Restarting Scanner...");
      ScannerOff();
      delay(10);
      ScannerOn();
      scannerStartTime = millis();
    }
  } else {
    if (scanner_active) {
      Serial.println("Scanner Deactivated");
      scanner_active = false;
      ScannerOff();
    }
  }
}
void ScannerOn() {
  digitalWrite(RELAY_PIN, HIGH);
}
void ScannerOff() {
  digitalWrite(RELAY_PIN, LOW);
}
//---------------------------------------------------------- END DOOR -----------------------------------------------

//---------------------------------------------------------- WIFI -----------------------------------------------
void setupWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected!");
}
//---------------------------------------------------------- END WIFI -----------------------------------------------


//---------------------------------------------------------- MQTT ---------------------------------------------------
void setupMQTT() {
    espClient.setCACert(root_ca);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loopMQTT() {
  if (!client.connected()) reconnect();
  client.loop();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
     if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT");

      // Subscribe to multiple topics
      client.subscribe("cpr/from_server/reservation");
      client.subscribe("cpr/from_server/reservation/display");

    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (String(topic) == "cpr/from_server/reservation") {
    if (message == "1") {
      openInDoor();
    } else {
      wrongKey();
    }
  } 
  else if (String(topic) == "cpr/from_server/reservation/display") {
    int colonIndex = message.indexOf(":");
    if (colonIndex != -1) {
      String slot = message.substring(0, colonIndex);
      String plate = message.substring(colonIndex + 1);

      Serial.print("Updating slot: ");
      Serial.print(slot);
      Serial.print(" with plate: ");
      Serial.println(plate);

      updateSlot(slot, plate);
  }
}
}

void publishJsonMQTT(String reservationId) {
  if (!client.connected()) {
    Serial.println("MQTT Disconnected. Reconnecting...");
    reconnect();
  }
  String jsonPayload = "{\"reservation_data\" : {\"id\":\" " + reservationId + "\"}}";
  String path = "cpr/from_board/reservation";
  client.publish(path.c_str(), jsonPayload.c_str());
}
//---------------------------------------------------------- END MQTT -----------------------------------------------



//---------------------------------------------------------- DISPLAY -----------------------------------------------

void displaySlots() {
  tft.loadFont("THSarabunPSK-Bold-70", SPIFFS);
  tft.setTextColor(TFT_BLACK);

  for (int i = 0; i < 6; i++) {
    tft.setCursor(15, 155 + (i * 55));
    tft.println(parkingSlots[i].slot + " : " + parkingSlots[i].plate);
  }
}

void updateSlot(String slotKey, String newPlate) {
  tft.fillScreen(TFT_WHITE);

  int16_t rc = png.openFLASH((uint8_t*)LOGO, sizeof(LOGO), pngDraw);
  if (rc == PNG_SUCCESS) {
    Serial.println("Successfully opened PNG file");
    tft.startWrite();
  }
  for (int i = 0; i < 6; i++) {
    if (parkingSlots[i].slot == slotKey) {
      parkingSlots[i].plate = newPlate;
      displaySlots(); 
      return;
    }
  }

}

void pngDraw(PNGDRAW* pDraw) {
  uint16_t lineBuffer[MAX_IMAGE_WIDTH];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(0, pDraw->y, pDraw->iWidth, 1, lineBuffer);
}
//---------------------------------------------------------- END DISPLAY -----------------------------------------------
