#include <Wire.h>
#include <Adafruit_HTU21DF.h>
#include <Adafruit_MPL115A2.h>
#include <Adafruit_TSL2591.h>
#include <ArduinoBLE.h>

// ------------------- SENSOR OBJECTS -------------------
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
Adafruit_MPL115A2 mpl = Adafruit_MPL115A2();
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);

// ------------------- BLE SETUP ------------------------
BLEService sensorService("180A"); // Custom service UUID
BLECharacteristic sensorCharacteristic("2A58", BLERead | BLENotify, 128); // 128-byte buffer

// ------------------- SETUP ----------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Initializing Sensors...\n");
  Wire.begin();

  // ---- HTU21D-F ----
  Serial.print("HTU21D-F: ");
  if (!htu.begin()) Serial.println("FAILED"); else Serial.println("OK");

  // ---- MPL115A2 ----
  Serial.print("MPL115A2: ");
  if (!mpl.begin()) Serial.println("FAILED"); else Serial.println("OK");

  // ---- TSL2591 ----
  Serial.print("TSL2591: ");
  if (!tsl.begin()) Serial.println("FAILED"); 
  else {
    Serial.println("OK");
    tsl.setGain(TSL2591_GAIN_MED);
    tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);
  }

  // ---- BLE ----
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }

  BLE.setLocalName("Nano33_Sensors");
  BLE.setAdvertisedService(sensorService);
  sensorService.addCharacteristic(sensorCharacteristic);
  BLE.addService(sensorService);
  sensorCharacteristic.writeValue("Init");

  BLE.advertise();
  Serial.println("BLE device active, waiting for connection...");
}

// ------------------- LOOP -----------------------------
void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    while (central.connected()) {
      // ---- Read sensors ----
      float temperature = htu.readTemperature();
      float humidity = htu.readHumidity();
      float pressure_kPa = mpl.getPressure();
      float mpl_tempC = mpl.getTemperature();
      uint32_t lum = tsl.getFullLuminosity();
      uint16_t ir = lum >> 16;
      uint16_t full = lum & 0xFFFF;

      // ---- Create data string ----
      char dataBuffer[128];
      snprintf(dataBuffer, sizeof(dataBuffer),
               "Temp_HTU:%.2f,Hum_HTU:%.2f,Pressure:%.2f,Temp_MPL:%.2f,Light_Full:%d,Light_IR:%d",
               temperature, humidity, pressure_kPa, mpl_tempC, full, ir);

      // ---- Send via BLE ----
      sensorCharacteristic.writeValue(dataBuffer);

      // ---- Debug on Serial ----
      Serial.println(dataBuffer);

      delay(1000);
    }

    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}




