#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>

//TODO: Value in EEPROM

#define LED_PIN       6
#define LED_NUM       30

Adafruit_NeoPixel ledstrip(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

BLEService            cradlesmartlightService     ("c9ea4800-ad9e-4d67-b570-69352fdc1078");
BLEByteCharacteristic ledstatusCharacteristic     ("c9ea4801-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite);
BLECharacteristic     ledcolorCharacteristic      ("c9ea4802-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite, 3);
BLEByteCharacteristic ledbrightnessCharacteristic ("c9ea4803-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite);

bool          led_status      = false;
unsigned char led_brightness  = 0;
unsigned char led_color_red   = 0;
unsigned char led_color_green = 0;
unsigned char led_color_blue  = 0;

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
      // Check LedStatus characteristic write
      if (ledstatusCharacteristic.written()) {
        if (ledstatusCharacteristic.value()) {   
          Serial.println("LED on");
          led_status = true;
          
          ledstrip.clear();
          ledstrip.setBrightness(led_brightness);
          for (int led_i=0; led_i<LED_NUM; led_i++) {
            ledstrip.setPixelColor(led_i, ledstrip.Color(led_color_red, led_color_green, led_color_blue));
          }
          ledstrip.show();
        } else {                              
          Serial.println(F("LED off"));
          led_status = false;
          
          ledstrip.clear();
          ledstrip.show();
        }
      }
      
      // Check LedColor characteristic write
      if (ledcolorCharacteristic.written()) {
        if (ledcolorCharacteristic.valueLength() == 3) {
          unsigned char dataRawColor[3] = {};
          ledcolorCharacteristic.readValue(dataRawColor, 42);

          led_color_red   = dataRawColor[0];
          led_color_green = dataRawColor[1];
          led_color_blue  = dataRawColor[2];

          for (int led_i=0; led_i<LED_NUM; led_i++) {
            ledstrip.setPixelColor(led_i, ledstrip.Color(led_color_red, led_color_green, led_color_blue));
          }

          if (led_status) ledstrip.show();
        }
      }

      // Check LedBrightness characteristic write
      if (ledbrightnessCharacteristic.written()) {
        led_brightness = ledbrightnessCharacteristic.value();
        ledstrip.setBrightness(led_brightness);
        
        if (led_status) ledstrip.show();
      }

    }

    // when the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
  }
}
