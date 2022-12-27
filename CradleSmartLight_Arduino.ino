#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN       6
#define LED_NUM       30

Adafruit_NeoPixel ledstrip(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

BLEService ledService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service
// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEByteCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // init LED strip
  ledstrip.begin();
  ledstrip.clear();
  ledstrip.show();

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("CradleSmartLight");
  BLE.setAdvertisedService(ledService);

  // add the characteristic to the service
  ledService.addCharacteristic(switchCharacteristic);

  // add service
  BLE.addService(ledService);

  // set the initial value for the characeristic:
  switchCharacteristic.writeValue(0);

  // start advertising
  BLE.advertise();

  Serial.println("BLE LED Peripheral");
}

void loop() {
  // listen for Bluetooth® Low Energy peripherals to connect:
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    // while the central is still connected to peripheral:
    while (central.connected()) {
      // if the remote device wrote to the characteristic,
      // use the value to control the LED:
      if (switchCharacteristic.written()) {
        if (switchCharacteristic.value()) {   // any value other than 0
          Serial.println("LED on");
          
          ledstrip.clear();
          ledstrip.setBrightness(255);
          for (int led_i=0; led_i<LED_NUM; led_i++) {
            ledstrip.setPixelColor(led_i, ledstrip.Color(255, 255, 255));
          }
          ledstrip.show();
        } else {                              // a 0 value
          Serial.println(F("LED off"));
          
          ledstrip.clear();
          ledstrip.show();
        }
      }
    }

    // when the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
  }
}
