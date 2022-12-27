#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>
#include <NanoBLEFlashPrefs.h>

#define LED_PIN       6
#define LED_NUM       30

Adafruit_NeoPixel ledstrip(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

BLEService            cradlesmartlightService     ("c9ea4800-ad9e-4d67-b570-69352fdc1078");
BLEByteCharacteristic ledstatusCharacteristic     ("c9ea4801-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite);
BLECharacteristic     ledcolorCharacteristic      ("c9ea4802-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite, 3);
BLEByteCharacteristic ledbrightnessCharacteristic ("c9ea4803-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite);

NanoBLEFlashPrefs myFlashPrefs;

typedef struct flashStruct
{
  bool led_status;
  unsigned char led_brightness;
  unsigned char led_color_red;
  unsigned char led_color_green;
  unsigned char led_color_blue;
} flashPrefs;

flashPrefs prefs;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // init LED strip
  ledstrip.begin();
  ledstrip.clear();
  ledstrip.show();
  
  Serial.println("Read record:");
  int rc = myFlashPrefs.readPrefs(&prefs, sizeof(prefs));
  if (rc == FDS_SUCCESS)
  {
    Serial.println("Preferences found: ");
    Serial.println(prefs.led_status);
    Serial.println(prefs.led_brightness);
    Serial.println(prefs.led_color_red);
    Serial.println(prefs.led_color_green);
    Serial.println(prefs.led_color_blue);
  } else {
    Serial.print("No preferences found. Return code: ");
    Serial.print(rc);
    Serial.print(", ");
    Serial.println(myFlashPrefs.errorString(rc));
    prefs.led_status = false;
    prefs.led_brightness = 0;
    prefs.led_color_red = 0;
    prefs.led_color_green = 0;
    prefs.led_color_blue = 0;
  }
  Serial.println("");

  ledstrip.setBrightness(prefs.led_brightness);
  for (int led_i=0; led_i<LED_NUM; led_i++) {
    ledstrip.setPixelColor(led_i, ledstrip.Color(prefs.led_color_red, prefs.led_color_green, prefs.led_color_blue));
  }
  if(prefs.led_status) ledstrip.show();

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
          prefs.led_status = true;
          
          ledstrip.clear();
          ledstrip.setBrightness(prefs.led_brightness);
          for (int led_i=0; led_i<LED_NUM; led_i++) {
            ledstrip.setPixelColor(led_i, ledstrip.Color(prefs.led_color_red, prefs.led_color_green, prefs.led_color_blue));
          }
          ledstrip.show();

          myFlashPrefs.writePrefs(&prefs, sizeof(prefs));
        } else {                              
          Serial.println(F("LED off"));
          prefs.led_status = false;
          
          ledstrip.clear();
          ledstrip.show();

          myFlashPrefs.writePrefs(&prefs, sizeof(prefs));
        }
      }
      
      // Check LedColor characteristic write
      if (ledcolorCharacteristic.written()) {
        if (ledcolorCharacteristic.valueLength() == 3) {
          unsigned char dataRawColor[3] = {};
          ledcolorCharacteristic.readValue(dataRawColor, 42);

          prefs.led_color_red   = dataRawColor[0];
          prefs.led_color_green = dataRawColor[1];
          prefs.led_color_blue  = dataRawColor[2];

          for (int led_i=0; led_i<LED_NUM; led_i++) {
            ledstrip.setPixelColor(led_i, ledstrip.Color(prefs.led_color_red, prefs.led_color_green, prefs.led_color_blue));
          }

          if (prefs.led_status) ledstrip.show();
          
          myFlashPrefs.writePrefs(&prefs, sizeof(prefs));
        }
      }

      // Check LedBrightness characteristic write
      if (ledbrightnessCharacteristic.written()) {
        prefs.led_brightness = ledbrightnessCharacteristic.value();
        ledstrip.setBrightness(prefs.led_brightness);
        
        if (prefs.led_status) ledstrip.show();

        myFlashPrefs.writePrefs(&prefs, sizeof(prefs));
      }

    }

    // when the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
  }
}
