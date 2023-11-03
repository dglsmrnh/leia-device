/*  
 *  Basic example to show how to read a BMP image from SPIFFS
 *  and display using Adafruit GFX
 *  
 *  Tested with esp32 devboard and 160x128 ST7735 display
 *  
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

// Display interface configuration
#define TFT_CS_PIN    GPIO_NUM_5
#define TFT_DC_PIN    GPIO_NUM_17
#define TFT_MOSI_PIN  GPIO_NUM_23
#define TFT_SCLK_PIN  GPIO_NUM_18
#define TFT_RST_PIN   GPIO_NUM_16
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS_PIN, TFT_DC_PIN, TFT_MOSI_PIN, TFT_SCLK_PIN, TFT_RST_PIN);

// Display backlight enable pin
#define TFT_BACKLIGHT_PIN GPIO_NUM_4

BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pWriteCharacteristic;
BLECharacteristic* pReadCharacteristic;

// Image reader
SPIFFS_ImageReader reader;

void displayImage(char* fileName) {
  // Open the BMP image file from SPIFFS
  File file = SPIFFS.open(fileName, "r");
  if (!file) {
    Serial.println("Failed to open image file.");
    return;
  }

  // Display the BMP image on the TFT display
  tft.fillScreen(ST77XX_BLACK); // Clear the screen
  // draw image
  reader.drawBMP(fileName, tft, 0, 0);
  reader.drawBMP(fileName, tft, 90, 0);

  file.close();
}

void saveImageAndDisplay(char* base64code) {
  // Check if base64code is present
  if (base64code) {
    // Decode the base64 data to binary data
    int inputStringLength = strlen(base64code);
    int decodedLength = Base64.decodedLength(base64code, inputStringLength);
    char decodedData[decodedLength + 1];
    Base64.decode(decodedData, base64code, inputStringLength);
    Serial.print("Decoded string is:\t");
    Serial.println(decodedData);

    // Save the binary data to a file (e.g., "image.bmp") in SPIFFS
    File file = SPIFFS.open("/image.bmp", "w");
    if (file) {
      file.write((uint8_t*)decodedData, decodedLength);
      file.close();
      Serial.println("Image saved to SPIFFS.");

      // Display the saved BMP image on the TFT display
      displayImage("/image.bmp");
    } else {
      Serial.println("Failed to open file for writing.");
    }
    delete[] decodedData;
  } else {
    Serial.println("base64code attribute not found in JSON.");
  }
}

void deserializeAndDisplayImage(std::string jsonStr) {
  // Create a JSON document
  DynamicJsonDocument jsonDocument(1024);

  // Deserialize the received JSON data
  DeserializationError error = deserializeJson(jsonDocument, jsonStr);

  if (error) {
    Serial.print("JSON deserialization failed: ");
    Serial.println(error.c_str());
    return;
  }

  File fileJson = SPIFFS.open("/data.json", "w");
  if (fileJson) {
    fileJson.write((uint8_t*)jsonStr.c_str(), jsonStr.length());
    fileJson.close();
    Serial.println("JSON saved to SPIFFS.");
  } else {
    Serial.println("Failed to open JSON file for writing.");
  }

  // Extract the base64code attribute from the JSON
  JsonObject data = jsonDocument["data"];
  JsonObject character = data["character"];
  JsonArray images = character["images"];
  char* base64code = strdup(images[0]["base64code"]);
  
  saveImageAndDisplay(base64code);

  free(base64code);
}

void serializeData() {
  StaticJsonDocument<256> jsonDocument;

  // Populate the JSON data
  jsonDocument["data"]["username"] = "JohnDoe";
  jsonDocument["data"]["inventory"][0]["id"] = "item1";
  jsonDocument["data"]["inventory"][0]["quantity"] = 5;

  // Serialize the JSON to a string
  std::string jsonString;
  serializeJson(jsonDocument, jsonString);

  // Set the JSON string as the value of the read characteristic
  pReadCharacteristic->setValue(jsonString);
}

class MyWriteCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    // Process the received JSON data (value) from the app
    deserializeAndDisplayImage(value);
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

  tft.fillScreen(ST77XX_WHITE);

  BLEDevice::init("ESP32_BLE");
  pServer = BLEDevice::createServer();
  pService = pServer->createService("1b9d0504-79f2-11ee-b962-0242ac120002");

  pReadCharacteristic = pService->createCharacteristic("e7ca6c9c-79f3-11ee-b962-0242ac120002", BLECharacteristic::PROPERTY_READ);
  pReadCharacteristic->setValue("{\"success\":false}");

  pWriteCharacteristic = pService->createCharacteristic("25bbac34-79f2-11ee-b962-0242ac120002", BLECharacteristic::PROPERTY_WRITE);
  pWriteCharacteristic->addDescriptor(new BLE2902());
  pWriteCharacteristic->setCallbacks(new MyWriteCallbacks());

  pService->start();
  pServer->getAdvertising()->start();
}

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void loop() {
  tft.fillScreen(ST77XX_WHITE);
  testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST77XX_BLACK);
  delay(1000);
  displayImage("/robot.bmp");
  delay(10000);
}
