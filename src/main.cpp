#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi credentials
char SSID[] = "Wokwi-GUEST";
char PASSWORD[] = "";

// MQTT Broker settings
const char* mqtt_server = "44.223.43.74";
const int mqtt_port = 1883;
const char* mqtt_topic = "/TEF/posture001/attrs/jsonObject";

WiFiClient espClient;
PubSubClient client(espClient);

// Define pins for Ultrasonic Sensor 1 (Back Support)
#define TRIG_PIN_1 13
#define ECHO_PIN_1 12

// Define pins for Ultrasonic Sensor 2 (Side/Lean Detection)
#define TRIG_PIN_2 27
#define ECHO_PIN_2 26

// Define pins for Ultrasonic Sensor 3 (Seat Occupancy)
#define TRIG_PIN_3 33
#define ECHO_PIN_3 32

// Posture thresholds (in cm)
const int BACK_GOOD_MIN = 5;
const int BACK_GOOD_MAX = 15;
const int SIDE_GOOD_MAX = 20;
const int SEAT_OCCUPIED = 30;

// Timing
unsigned long lastCheck = 0;
const int CHECK_INTERVAL = 1000; // 1 Second

// Posture tracking
int goodPostureTime = 0;
int badPostureTime = 0;
unsigned long sessionStart = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize ultrasonic sensor pins
  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);
  pinMode(TRIG_PIN_3, OUTPUT);
  pinMode(ECHO_PIN_3, INPUT);
  
  Serial.println("=================================");
  Serial.println("ESP32 Posture Monitor with MQTT");
  Serial.println("=================================");
  
  // Connect to WiFi
  setup_wifi();
  
  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  
  sessionStart = millis();
}

void setup_wifi()
{
	delay(10);
	Serial.println("WiFi Connection");
	Serial.print("Connecting into the net: ");
	Serial.println(SSID);
	Serial.println("Wait");

	reconnectWiFi();
}

void reconnectWiFi()
{
	if (WiFi.status() == WL_CONNECTED)
	{
		return;
	}

	WiFi.begin(SSID, PASSWORD);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(100);
		Serial.print(".");
	}

	Serial.println();
	Serial.print("Connected with success into the net ");
	Serial.println(SSID);
	Serial.println("IP: ");
	Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  if (millis() - lastCheck >= CHECK_INTERVAL) {
    lastCheck = millis();
    
    // Read all sensors
    float backDistance = getDistance(TRIG_PIN_1, ECHO_PIN_1);
    float sideDistance = getDistance(TRIG_PIN_2, ECHO_PIN_2);
    float seatDistance = getDistance(TRIG_PIN_3, ECHO_PIN_3);
    
    // Analyze posture
    String postureStatus = analyzePosture(backDistance, sideDistance, seatDistance);
    
    // Update time tracking
    if (postureStatus == "good") {
      goodPostureTime++;
    } 
    else if (postureStatus == "bad") {
      badPostureTime++;
    }
    
    // Create JSON payload
    StaticJsonDocument<300> doc;
    doc["backDistance"] = round(backDistance * 10) / 10.0;
    doc["sideDistance"] = round(sideDistance * 10) / 10.0;
    doc["seatDistance"] = round(seatDistance * 10) / 10.0;
    doc["postureStatus"] = postureStatus;
    doc["goodTime"] = goodPostureTime;
    doc["badTime"] = badPostureTime;
    doc["sessionTime"] = (millis() - sessionStart) / 1000;
    
    // Calculate posture score (0-100)
    int totalTime = goodPostureTime + badPostureTime;
    int score = totalTime > 0 ? (goodPostureTime * 100) / totalTime : 100;
    doc["postureScore"] = score;
    
    // Serialize and publish
    char jsonBuffer[300];
    serializeJson(doc, jsonBuffer);
    
    client.publish(mqtt_topic, jsonBuffer);
    
    // Debug output
    Serial.println("Published:");
    Serial.println(jsonBuffer);
    Serial.println();
  }
}

float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  float distance = duration * 0.0343 / 2;
  
  if (distance == 0 || distance > 400) {
    return 400;
  }
  
  return distance;
}

String analyzePosture(float back, float side, float seat) {
  // Check if someone is sitting
  if (seat > SEAT_OCCUPIED) {
    return "empty";
  }
  
  // Check back distance and side lean
  bool backGood = (back >= BACK_GOOD_MIN && back <= BACK_GOOD_MAX);
  bool sideGood = (side <= SIDE_GOOD_MAX);
  
  if (backGood && sideGood) {
    return "good";
  } 
  else if (!backGood && !sideGood) {
    return "bad";
  }
  return "warning";
}