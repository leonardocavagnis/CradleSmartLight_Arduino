#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN       6
#define LED_NUM       30

Adafruit_NeoPixel ledstrip(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

BLEService            cradlesmartlightService     ("c9ea4800-ad9e-4d67-b570-69352fdc1078");
BLEByteCharacteristic ledstatusCharacteristic     ("c9ea4801-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite);
BLEByteCharacteristic ledcolorCharacteristic      ("c9ea4802-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite);
BLEByteCharacteristic ledbrightnessCharacteristic ("c9ea4803-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite);

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
  BLE.setAdvertisedService(cradlesmartlightService);

  // add the characteristics to the service
  cradlesmartlightService.addCharacteristic(ledstatusCharacteristic);
  cradlesmartlightService.addCharacteristic(ledcolorCharacteristic);
  cradlesmartlightService.addCharacteristic(ledbrightnessCharacteristic);
  
  // add service
  BLE.addService(cradlesmartlightService);

  // set the initial value for the characeristic:
  ledstatusCharacteristic.writeValue(0);
  ledcolorCharacteristic.writeValue(0);
  ledbrightnessCharacteristic.writeValue(0); 
  
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
      if (ledstatusCharacteristic.written()) {
        if (ledstatusCharacteristic.value()) {   // any value other than 0
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
