#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "mqtt_func.h"
#include "ir_and_led_setup.h"
#include "parking_slot.h"
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

WiFiClientSecure espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

Parking_slots parking_slot[] = {
  { "A1", "F1", a1_ir_sensor, "1", a1_led_green, a1_led_red, a1_led_yellow },
  { "A2", "F1", a2_ir_sensor, "1", a2_led_green, a2_led_red, a2_led_yellow },
  { "A3", "F1", a3_ir_sensor, "1", a3_led_green, a3_led_red, a3_led_yellow },
  { "B1", "F1", b1_ir_sensor, "1", b1_led_green, b1_led_red, b1_led_yellow },
  { "B2", "F1", b2_ir_sensor, "1", b2_led_green, b2_led_red, b2_led_yellow },
  { "B3", "F1", b3_ir_sensor, "1", b3_led_green, b3_led_red, b3_led_yellow }
};

const int parking_size = sizeof(parking_slot) / sizeof(parking_slot[0]);



void setup() {
  Serial.begin(115200);  // เริ่มต้น Serial Monitor
  setup_sensor_led();
  setup_wifi();
  setup_mqtt();

  while (!client.connected()) {
    Serial.println("Waiting for MQTT connection...");
    reconnect();
    delay(500);
  }

  get_parking_status();
}

void loop() {
  loop_mqtt();

  for (int i = 0; i < parking_size; i++) {
    int current_ir = digitalRead(parking_slot[i].pin);  

    if (String(current_ir) != parking_slot[i].old_value) { 
      if (parking_slot[i].old_value == "2" && String(current_ir) == "0") {
        publish_json_mqtt(parking_slot[i].floor, parking_slot[i].name, current_ir);
        parking_slot[i].old_value = current_ir;
      } else if (parking_slot[i].old_value == "0" && String(current_ir) == "1") {
        publish_json_mqtt(parking_slot[i].floor, parking_slot[i].name, current_ir);
        parking_slot[i].old_value = current_ir;
      }
    
    }

    if (parking_slot[i].old_value == "1") {
      digitalWrite(parking_slot[i].green, HIGH);
      digitalWrite(parking_slot[i].red, LOW);
      digitalWrite(parking_slot[i].yellow, LOW);
    } else if (parking_slot[i].old_value == "0") {
      digitalWrite(parking_slot[i].green, LOW);
      digitalWrite(parking_slot[i].red, HIGH);
      digitalWrite(parking_slot[i].yellow, LOW);
    } else if (parking_slot[i].old_value == "2"){
      digitalWrite(parking_slot[i].green, LOW);
      digitalWrite(parking_slot[i].red, LOW);
      digitalWrite(parking_slot[i].yellow, HIGH);
    }
    else{
         digitalWrite(parking_slot[i].green, LOW);
      digitalWrite(parking_slot[i].red, LOW);
      digitalWrite(parking_slot[i].yellow, LOW);
    }
  }

  Serial.println("sss : " + parking_slot[0].old_value);
  delay(1000);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}


void setup_mqtt() {
  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop_mqtt() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}


void callback(char* topic, byte* payload, unsigned int length) {
  int section = 0;
  String floor = "";
  String slot = "";
  String status = "";

  for (int i = 0; i < length; i++) {
    if ((char)payload[i] == ':') {
      section++;
      continue;
    }
    if (section == 0) {
      floor = floor + (char)payload[i];
    }
    if (section == 1) {
      slot = slot + (char)payload[i];
    }
    if (section == 2) {
      status = status + (char)payload[i];
    }
  }

  for (int i = 0; i < parking_size; i++) {
    if (parking_slot[i].name == slot && parking_slot[i].floor == floor) {
      if (parking_slot[i].old_value != status) {  
        parking_slot[i].old_value = status;
      }
    }
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
 
   if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
    
      // ... and resubscribe
      client.subscribe("cpr/from_server/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void publish_json_mqtt(String floor, String name, int status) {
  if (!client.connected()) {
    Serial.println("MQTT Disconnected. Reconnecting...");
    reconnect();
  }
  String str_status = status == 0 ? "FULL" : "IDLE";
  String jsonPayload = "{\"parking_data\" : {\"name\":\" " + name + "\",\"floor\":\"" + floor + "\",\"status\":\"" + str_status + "\"}}";
  String path = "cpr/from_board/slot/" + floor + "/" + name;
  client.publish(path.c_str(), jsonPayload.c_str());
}


void get_parking_status() {
  if (!client.connected()) {
    Serial.println("MQTT Disconnected. Reconnecting...");
    reconnect();
  }
  String path = "cpr/from_board/setup";
  client.publish(path.c_str(), "{\"data\" : {\"message\":\"setup\"}}");
}
