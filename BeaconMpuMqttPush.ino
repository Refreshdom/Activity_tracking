// LinkIt One sketch for MQTT Demo
#include <ArduinoJson.h>

#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"

#define buttonPin 5

MPU6050 accelgyro;

int16_t ax, ay, az;
int16_t gx, gy, gz;

#define OUTPUT_READABLE_ACCELGYRO

#include <LWiFi.h>
#include <PubSubClient.h>
#include <LBLE.h>
#include <LBLEPeriphral.h>
/*
  Modify to your WIFI Access Credentials.
*/

const char* ssid = "mmudigitalhome@unifi";
const char* password = "79044920";
IPAddress ip(192, 168, 0, 225);
/*
  Modify to your MQTT broker - Select only one
*/
char mqttBroker[] = "iotfoe.ddns.net";
// byte mqttBroker[] = {192,168,1,220}; // modify to your local broker

WiFiClient wifiClient;

PubSubClient client( wifiClient );

unsigned long lastSend;


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println( "[DONE]" );
      // Publish a message on topic "outTopic"
      client.publish( "outTopic", "Hello, This is LinkIt One" );
      // Subscribe to topic "inTopic"
//      client.subscribe( "inTopic" );
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}

void setup_wifi() {
  WiFi.config(ip);
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  delay( 4000 );
  pinMode(buttonPin,INPUT);
  //////////////////////////////////////////beacon setup////////////////////////////////////////////
  Serial.begin(115200);
  delay(500);

  // Initialize BLE subsystem
  Serial.println("BLE begin");
  LBLE.begin();
  while (!LBLE.ready()) {
    delay(100);
  }
  Serial.println("BLE ready");

  // configure our advertisement data as iBeacon.
  LBLEAdvertisementData beaconData;

  // This is a common AirLocate example UUID.
  LBLEUuid uuid("E2C56DB5-DFFB-48D2-B060-D0F5A71096E0");
  beaconData.configAsIBeacon(uuid, 01, 02, -40);

  Serial.print("Start advertising iBeacon with uuid=");
  Serial.println(uuid);

  // start advertising it
  LBLEPeripheral.advertise(beaconData);

  /////////////////////////////////////////////////////////////////////////////////////////////////
  setup_wifi();

  client.setServer( mqttBroker, 1883 );
  client.setCallback( callback );

  lastSend = 0;

  Wire.begin();   //begin I2c
  Serial.begin(115200);

  // initialize device
  Serial.println("Initializing I2C devices...");
  accelgyro.initialize();

  // verify connection
  Serial.println("Testing device connections...");
  Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
}

void loop()
{
  accelgyro.getAcceleration(&ax, &ay, &az);
  accelgyro.getRotation(&gx, &gy, &gz);

  if ( !client.connected() ) {
    reconnect();
  }

  //>>>>>>>>>>>>button setup for easy record raw data>>>>>>>>>>>
  if ( digitalRead(buttonPin) == HIGH ) {
    if ( millis() - lastSend > 100 ) { // Send an update only after 5 seconds
      sendSensorData();
      lastSend = millis();
      //        Serial.print("a/g:\t");
      //        Serial.print(ax); Serial.print("\t");
      //        Serial.print(ay); Serial.print("\t");
      //        Serial.print(az); Serial.print("\t");
      //        Serial.print(gx); Serial.print("\t");
      //        Serial.print(gy); Serial.print("\t");
      //        Serial.println(gz);
    }
  }

  client.loop();
}

void callback( char* topic, byte* payload, unsigned int length ) {
  Serial.print( "Recived message on Topic:" );
  Serial.print( topic );
  Serial.print( "    Message:");
  for (int i = 0; i < length; i++) {
    Serial.print( (char)payload[i] );
  }
  Serial.println();
}

void sendSensorData() {

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  // Just debug messages
  //  Serial.print( "Sending sensor data : [" );
  //  Serial.print(ax); Serial.print( ay ); Serial.print( az );Serial.print(gx); Serial.print( gy ); Serial.print( gz );
  //  Serial.print( "]   -> " );

  // Prepare a JSON payload string
  root["ax"] = ax; root["ay"] = ay; root["az"] = az;
  root["gx"] = gx; root["gy"] = gy; root["gz"] = gz;
  // Send payload
  char buffers[256];
  root.printTo(buffers, sizeof(buffers));
  client.publish("linkit/sensor/mpu", buffers);
//    Serial.println(buffers);
}

