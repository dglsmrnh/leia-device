/*  
 *  LEIA app device
 */

// Required libraries
#include <SPIFFS.h>
#include <FS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPIFFS_ImageReader.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <Base64.h>
#include <Information.h>

// Display interface configuration
#define TFT_CS_PIN    GPIO_NUM_5
#define TFT_DC_PIN    GPIO_NUM_17
#define TFT_MOSI_PIN  GPIO_NUM_23
#define TFT_SCLK_PIN  GPIO_NUM_18
#define TFT_RST_PIN   GPIO_NUM_16
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS_PIN, TFT_DC_PIN, TFT_MOSI_PIN, TFT_SCLK_PIN, TFT_RST_PIN);

// Display backlight enable pin
#define TFT_BACKLIGHT_PIN GPIO_NUM_4

// BLR services
BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pCharacteristic;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// Image reader
SPIFFS_ImageReader reader;

Information inf;

bool displayImage(char* fileName, int xPos, int yPos) {
  // Open the BMP image file from SPIFFS
  File file = SPIFFS.open(fileName, "r");
  if (!file) {
    Serial.println("Failed to open image file.");
    return false;
  }

  // Display the BMP image on the TFT display
  tft.fillScreen(ST77XX_BLACK); // Clear the screen
  // Draw image at the specified position
  reader.drawBMP(fileName, tft, xPos, yPos);

  file.close();

   return true;
}

// Function to read and parse JSON from SPIFFS
bool readJsonData() {
  File file = SPIFFS.open("/data.json", "r");

  if (file) {
    // Read the JSON content from the file
    size_t fileSize = file.size();
    std::unique_ptr<char[]> buf(new char[fileSize]);
    file.readBytes(buf.get(), fileSize);

    if(!inf.processJson(buf.get(), false)) {
      return false;
    }
    return true;
  } else {
    Serial.println("Error opening file for reading");
    return false;
  }
}

class BLECallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    Serial.println("Received data: ");
    Serial.println(value.c_str());
    
    if(!inf.processJson(value.c_str(), true)) {
      Serial.println("JSON process error");
    }
  }

  void onRead(BLECharacteristic* pCharacteristic) {
    pCharacteristic->setValue(inf.getCharacterInfoJson());
  }
};

void setup() {

  Serial.begin(115200);
  Serial.println(F("Hello! ST7735mini TFT Test"));

  // initialize SPIFFS
  if(!SPIFFS.begin()) {
    Serial.println("SPIFFS initialization failed!");
    while (1);
  }

  // initialize display and turn on backlight
  tft.initR(INITR_MINI160x80_PLUGIN);  // Init ST7735S mini display (when seeing inversed)
  // tft.initR(INITR_MINI160x80);  // Init ST7735S mini display
  tft.setRotation(1); // Set display rotation as needed
  Serial.println(F("Initialized"));
  delay(500);
  // pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
  // digitalWrite(TFT_BACKLIGHT_PIN, LOW);

  tft.fillScreen(ST77XX_BLACK);

  BLEDevice::init("ESP32_BLE");
  pServer = BLEDevice::createServer();
  pService = pServer->createService("1b9d0504-79f2-11ee-b962-0242ac120002");

  pCharacteristic = pService->createCharacteristic("e7ca6c9c-79f3-11ee-b962-0242ac120002", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new BLECallback());

  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("Waiting for a connection...");
  readJsonData();
  displayImage("/robot.bmp", 0, 0);
}

void loop() {
}
