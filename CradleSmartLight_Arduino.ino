#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>
#include <NanoBLEFlashPrefs.h>
#include "mbed.h"
#include <time.h>

#define LED_PIN       6
#define LED_NUM       30

#define PIR_PIN             3
#define PIR_MIN_BRIGHTNESS  10
#define PIR_MAX_BRIGHTNESS  200

Adafruit_NeoPixel     ledstrip(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

BLEService            cradlesmartlightService     ("c9ea4800-ad9e-4d67-b570-69352fdc1078");
BLEByteCharacteristic ledstatusCharacteristic     ("c9ea4801-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite);
BLECharacteristic     ledcolorCharacteristic      ("c9ea4802-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite, 3);
BLEByteCharacteristic ledbrightnessCharacteristic ("c9ea4803-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite);
BLEByteCharacteristic pirstatusCharacteristic     ("c9ea4804-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite);
BLECharacteristic     currenttimeCharacteristic   ("c9ea4805-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite, 6);
BLECharacteristic     timerfeatureCharacteristic  ("c9ea4806-ad9e-4d67-b570-69352fdc1078", BLERead | BLEWrite, 5);

NanoBLEFlashPrefs     myFlashPrefs;

typedef struct flashStruct
{
  bool led_status;
  byte led_brightness;
  byte led_color_rgb[3];
  bool pir_status;
  bool timer_status;
  byte timer_on_hh;
  byte timer_on_mm;
  byte timer_off_hh;
  byte timer_off_mm;
} flashPrefs;

flashPrefs prefs;

bool          pir_val           = 0;
bool          pir_val_prev      = 0;
bool          pir_enabled       = false;
unsigned long pir_turnon_millis = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // init PIR sensor
  pinMode(PIR_PIN, INPUT); 
  
  // init LED strip
  ledstrip.begin();

  // read persistent memory for parameters
  Serial.println("Read memory...");
  int rc = myFlashPrefs.readPrefs(&prefs, sizeof(prefs));
  if (rc == FDS_SUCCESS) {
    Serial.println("Data found: ");
    Serial.println(prefs.led_status);
    Serial.println(prefs.led_brightness);
    Serial.println(prefs.led_color_rgb[0]);
    Serial.println(prefs.led_color_rgb[1]);
    Serial.println(prefs.led_color_rgb[2]);
    Serial.println(prefs.pir_status);
    Serial.println(prefs.timer_status);
    Serial.println(prefs.timer_on_hh);
    Serial.println(prefs.timer_on_mm);
    Serial.println(prefs.timer_off_hh);
    Serial.println(prefs.timer_off_mm);
  } else {
    Serial.println("Data not found: set default.");
    prefs.led_status        = false;
    prefs.led_brightness    = 0;
    prefs.led_color_rgb[0]  = 0;
    prefs.led_color_rgb[1]  = 0;
    prefs.led_color_rgb[2]  = 0;
    prefs.pir_status        = false;
    prefs.timer_status      = false;
    prefs.timer_on_hh       = 0;
    prefs.timer_on_mm       = 0;
    prefs.timer_off_hh      = 0;
    prefs.timer_off_mm      = 0;
  }

  if (prefs.led_status) {
      if (prefs.pir_status) ledstrip_on(PIR_MIN_BRIGHTNESS, prefs.led_color_rgb[0], prefs.led_color_rgb[1], prefs.led_color_rgb[2]);
      else ledstrip_on(prefs.led_brightness, prefs.led_color_rgb[0], prefs.led_color_rgb[1], prefs.led_color_rgb[2]);
  } else {
      ledstrip_off();
  }

  // BLE initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE module failed!");
    while (1);
  }

  // set advertised local name and service UUID
  BLE.setLocalName("CradleSmartLight");
  BLE.setAdvertisedService(cradlesmartlightService);

  // add the characteristics to the CradleSmartLight service
  cradlesmartlightService.addCharacteristic(ledstatusCharacteristic);
  cradlesmartlightService.addCharacteristic(ledcolorCharacteristic);
  cradlesmartlightService.addCharacteristic(ledbrightnessCharacteristic);
  cradlesmartlightService.addCharacteristic(pirstatusCharacteristic);
  cradlesmartlightService.addCharacteristic(currenttimeCharacteristic);
  cradlesmartlightService.addCharacteristic(timerfeatureCharacteristic);
  
  // add CradleSmartLight service
  BLE.addService(cradlesmartlightService);

  // get current time
  time_t seconds = time( NULL );
  struct tm * currentTime;
  currentTime = localtime(&seconds);
  byte currenttime_init[6];
  currenttime_init[0] = currentTime->tm_hour;
  currenttime_init[1] = currentTime->tm_min;
  currenttime_init[2] = currentTime->tm_sec;
  currenttime_init[3] = currentTime->tm_mday;
  currenttime_init[4] = currentTime->tm_mon + 1;
  currenttime_init[5] = (currentTime->tm_year + 1900) - 2000;

  // get timer parameter
  byte timerfeature_init[5];
  timerfeature_init[0] = prefs.timer_status;
  timerfeature_init[1] = prefs.timer_on_hh;
  timerfeature_init[2] = prefs.timer_on_mm;
  timerfeature_init[3] = prefs.timer_off_hh;
  timerfeature_init[4] = prefs.timer_off_mm;
  
  // set the initial value for the characeristics
  ledstatusCharacteristic.writeValue(prefs.led_status);
  ledcolorCharacteristic.writeValue(prefs.led_color_rgb, 3);
  ledbrightnessCharacteristic.writeValue(prefs.led_brightness); 
  pirstatusCharacteristic.writeValue(prefs.pir_status);
  currenttimeCharacteristic.writeValue(currenttime_init, 6);
  timerfeatureCharacteristic.writeValue(timerfeature_init, 5);

  // set read request handler for CurrentTime characteristic
  currenttimeCharacteristic.setEventHandler(BLERead, currenttimeCharacteristicRead);
  
  // start advertising
  BLE.advertise();

  Serial.println("CradleSmartLight program starts");
}

void loop() {
  // Check PIR movement (when BLE is disconnected)
  timer_pir_handler();
  
  // listen for BLE peripherals to connect
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    // while the central is still connected to peripheral:
    while (central.connected()) {
      // Check PIR movement (when BLE is connected)
      timer_pir_handler();

      // Check LedStatus characteristic write
      if (ledstatusCharacteristic.written()) {
        if (ledstatusCharacteristic.value()) {   
          Serial.println("LED on");

          if (prefs.led_status == false) {
            prefs.led_status = true;
            
            if (prefs.pir_status) ledstrip_on(PIR_MIN_BRIGHTNESS, prefs.led_color_rgb[0], prefs.led_color_rgb[1], prefs.led_color_rgb[2]);
            else ledstrip_on(prefs.led_brightness, prefs.led_color_rgb[0], prefs.led_color_rgb[1], prefs.led_color_rgb[2]);

            myFlashPrefs.writePrefs(&prefs, sizeof(prefs));
          }  
        } else {                              
          Serial.println("LED off");

          if (prefs.led_status == true) {
            prefs.led_status = false;
            pir_enabled = false;
            ledstrip_off();

            myFlashPrefs.writePrefs(&prefs, sizeof(prefs));
          } 
        }
      }
      
      // Check LedColor characteristic write
      if (ledcolorCharacteristic.written()) {
        if (ledcolorCharacteristic.valueLength() == 3) {
          byte dataRawColor[3] = {};
          ledcolorCharacteristic.readValue(dataRawColor, 3);

          prefs.led_color_rgb[0] = dataRawColor[0];
          prefs.led_color_rgb[1] = dataRawColor[1];
          prefs.led_color_rgb[2] = dataRawColor[2];

          for (int led_i=0; led_i<LED_NUM; led_i++) {
            ledstrip.setPixelColor(led_i, ledstrip.Color(prefs.led_color_rgb[0], prefs.led_color_rgb[1], prefs.led_color_rgb[2]));
          }

          if (prefs.led_status) ledstrip.show();
          
          myFlashPrefs.writePrefs(&prefs, sizeof(prefs));
        }
      }

      // Check LedBrightness characteristic write
      if (ledbrightnessCharacteristic.written()) {
        prefs.led_brightness = ledbrightnessCharacteristic.value();
        
        if (prefs.led_status && !prefs.pir_status) ledstrip_on(prefs.led_brightness, prefs.led_color_rgb[0], prefs.led_color_rgb[1], prefs.led_color_rgb[2]);

        myFlashPrefs.writePrefs(&prefs, sizeof(prefs));
      }

      // Check PIRStatus characteristic write
      if (pirstatusCharacteristic.written()) {
        prefs.pir_status = pirstatusCharacteristic.value();
        
        if (prefs.led_status) {
          if(prefs.pir_status) {
            ledstrip_on(PIR_MIN_BRIGHTNESS, prefs.led_color_rgb[0], prefs.led_color_rgb[1], prefs.led_color_rgb[2]);
          } else {
            ledstrip_on(prefs.led_brightness, prefs.led_color_rgb[0], prefs.led_color_rgb[1], prefs.led_color_rgb[2]);
          }
        }

        myFlashPrefs.writePrefs(&prefs, sizeof(prefs));
      }
      
      // Check CurrentTime characteristic write
      if (currenttimeCharacteristic.written()) {
        if (currenttimeCharacteristic.valueLength() == 6) {
          byte currentTimeFromBle[6] = {};
          currenttimeCharacteristic.readValue(currentTimeFromBle, 6);
          
          struct tm currentTime;
          
          currentTime.tm_hour = currentTimeFromBle[0];                  // hour (0-23)
          currentTime.tm_min  = currentTimeFromBle[1];                  // minutes (0-59)
          currentTime.tm_sec  = currentTimeFromBle[2];                  // seconds (0-59)
          currentTime.tm_mday = currentTimeFromBle[3];                  // day of month (1-31)
          currentTime.tm_mon  = currentTimeFromBle[4] - 1;              // month are from (0-11)
          currentTime.tm_year = (2000 + currentTimeFromBle[5]) - 1900;  // years since 1900
 
          set_time(mktime(&currentTime));

          time_t seconds = time( NULL );
          Serial.println(asctime(localtime(&seconds)));
        }
      }

      // Check TimerFeature characteristic write
      if (timerfeatureCharacteristic.written()) {
        if (timerfeatureCharacteristic.valueLength() == 5) {
          byte timerFeatureData[5] = {};
          timerfeatureCharacteristic.readValue(timerFeatureData, 5);
          
          prefs.timer_status      = timerFeatureData[0];
          prefs.timer_on_hh       = timerFeatureData[1];
          prefs.timer_on_mm       = timerFeatureData[2];
          prefs.timer_off_hh      = timerFeatureData[3];
          prefs.timer_off_mm      = timerFeatureData[4];
          
          myFlashPrefs.writePrefs(&prefs, sizeof(prefs));
        }
      }
    }

    // when the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
  }
}

void ledstrip_on(byte brightness, byte color_r, byte color_g, byte color_b){
  ledstrip.clear();     
  ledstrip.setBrightness(brightness);
  for (int led_i=0; led_i<LED_NUM; led_i++) {
    ledstrip.setPixelColor(led_i, ledstrip.Color(color_r, color_g, color_b));
  }
  ledstrip.show();
}

void ledstrip_off(){
  ledstrip.clear();
  ledstrip.show();
}

void pir_handler(){
  pir_val = digitalRead(PIR_PIN);
  
  if (pir_val_prev == 0 && pir_val == 1 && prefs.pir_status == true && prefs.led_status == true) {
      Serial.println("Move detected!");
      pir_enabled           = true;
      pir_turnon_millis     = millis();
      
      ledstrip_on(PIR_MAX_BRIGHTNESS, prefs.led_color_rgb[0], prefs.led_color_rgb[1], prefs.led_color_rgb[2]);
  }

  if (pir_enabled) {
    if (millis() - pir_turnon_millis >= 30000) {
      Serial.println("PIR Timeout expired");
      pir_enabled           = false;
      
      ledstrip_on(PIR_MIN_BRIGHTNESS, prefs.led_color_rgb[0], prefs.led_color_rgb[1], prefs.led_color_rgb[2]);
    }
  }
  
  pir_val_prev = pir_val;
}

void currenttimeCharacteristicRead(BLEDevice central, BLECharacteristic characteristic) {
  time_t seconds = time( NULL );
  struct tm * currentTime;
  currentTime = localtime(&seconds);
  byte currenttime_init[6];
  
  currenttime_init[0] = currentTime->tm_hour;
  currenttime_init[1] = currentTime->tm_min;
  currenttime_init[2] = currentTime->tm_sec;
  currenttime_init[3] = currentTime->tm_mday;
  currenttime_init[4] = currentTime->tm_mon + 1;
  currenttime_init[5] = (currentTime->tm_year + 1900) - 2000;
  
  currenttimeCharacteristic.writeValue(currenttime_init, 6);

  Serial.println(asctime(localtime(&seconds)));
}

void timer_pir_handler(){
  if (prefs.led_status == true) {
    if (prefs.pir_status == true && prefs.timer_status == false) {
      pir_handler();
    } else if (prefs.timer_status == true) {
      time_t seconds = time( NULL );
      struct tm * currentTime;
      currentTime = localtime(&seconds);
      
      if (check_hhmm_interval(currentTime->tm_hour, currentTime->tm_min, prefs.timer_on_hh, prefs.timer_on_mm, prefs.timer_off_hh, prefs.timer_off_mm)) {
        if (prefs.pir_status == true) {
          pir_handler();
        } else {
          ledstrip_on(prefs.led_brightness, prefs.led_color_rgb[0], prefs.led_color_rgb[1], prefs.led_color_rgb[2]);
        }
      } else {
        ledstrip_off();
      }
    } else {
      ledstrip_on(prefs.led_brightness, prefs.led_color_rgb[0], prefs.led_color_rgb[1], prefs.led_color_rgb[2]);
    }
  } else {
    ledstrip_off();
  }
}

bool check_hhmm_interval(byte check_hour, byte check_minute, byte start_hour, byte start_minute, byte end_hour, byte end_minute){   
  if ((end_hour > start_hour) || (end_hour == start_hour && end_minute >= start_minute)) {
      if ((check_hour > start_hour && check_hour < end_hour)                                        ||
          (check_hour == start_hour && check_minute >= start_minute && check_minute <= end_minute)  ||
          (check_hour == end_hour && check_minute <= end_minute)      ) {
        return true;
    } else {
        return false;
    }
  } else {
    if ((check_hour > end_hour && check_hour < start_hour)        ||
        (check_hour == end_hour && check_minute > end_minute)     ||
        (check_hour == start_hour && check_minute < start_minute)   ) {
        return false;
    } else {
        return true;
    }  
  }
}
