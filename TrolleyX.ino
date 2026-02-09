#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include "HX711.h"
#include <Firebase_ESP_Client.h>

#define WIFI_NAME "Kadam"
#define WIFI_PASSWORD "Pranaya1234"
#define API_KEY "AIzaSyD74e_gSKO4ebyUn8inQyqUAMyrT7xvJ8E"
#define DATABASE_URL "https://trolleyx-it-default-rtdb.firebaseio.com/"
#define SDA 21
#define RST 22
MFRC522 rfid(SDA, RST);
#define HX_DT 4
#define HX_SCK 5
HX711 scale;
float calibration_factor=193000;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
void tokenStatusCallback(TokenInfo info)
{
  Serial.printf("Token status: %s\n",
    info.status == token_status_ready ? "READY" : "NOT READY");
}

void setup() 
{
  Serial.begin(115200);
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  if (!Firebase.signUp(&config, &auth, "", "")) 
  {
    Serial.println("Anonymous Sign-In Failed");
    Serial.println(config.signer.signupError.message.c_str());
  } else {
    Serial.println("Anonymous Sign-In Success");
  }
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  while (!Firebase.ready()) delay(100);
  Serial.println("Firebase Ready");

  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID Ready");
  scale.begin(HX_DT, HX_SCK);
  scale.set_scale(calibration_factor);
  scale.tare();
  Serial.println("Load Cell Ready");
}

void loop() 
{
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;
  String rfidTag = "";
  for (byte i = 0; i < rfid.uid.size; i++) 
  {
    rfidTag += String(rfid.uid.uidByte[i], HEX);
  }
  rfidTag.toUpperCase();
  Serial.print("RFID Tag: ");
  Serial.println(rfidTag);

  float weight = scale.get_units(1);
  if (weight < 0) weight = 0;
  Serial.print("Weight: ");
  Serial.print(weight);
  Serial.println(" kg");

  String path = "/trolley1/items/" + rfidTag;
  Firebase.RTDB.setString(&fbdo, path + "/rfid", rfidTag);
  Firebase.RTDB.setFloat(&fbdo, path + "/weight", weight);
  Serial.println("Data sent to Firebase");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(2000);
}
