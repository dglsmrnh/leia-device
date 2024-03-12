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

#define JOY_X_PIN  GPIO_NUM_35
#define JOY_Y_PIN  GPIO_NUM_34
// #define JOY_BTN_PIN 4

#define BTN_BACK_PIN GPIO_NUM_33
#define BTN_START_PIN GPIO_NUM_32 

// BLR services
BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pCharacteristic;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// Image reader
SPIFFS_ImageReader reader;

Information inf;

#define NUM_BUTTONS 2 // Number of menu buttons
// Button colors
#define COLOR_NORMAL ST7735_WHITE
#define COLOR_SELECTED ST7735_BLUE

#define MENU_AREA_H 40
#define MENU_AREA_W 80
#define FOOTER_AREA 10

#define CHARACTER_X 90
#define CHARACTER_Y 5

const char* menuItems[NUM_BUTTONS] = {"Inventario", "Sincronizar"};
int selectedButton = 0; // Index of the currently selected button
bool homeScreen = true; // Indicates whether the home screen is being displayed

void clearScreen() {
  tft.fillScreen(ST77XX_BLACK); // Clear the screen
}

bool displayImage(char* fileName, int xPos, int yPos) {
  // Open the BMP image file from SPIFFS
  File file = SPIFFS.open(fileName, "r");
  if (!file) {
    Serial.println("Failed to open image file.");
    return false;
  }

  // Display the BMP image on the TFT display
  // tft.fillScreen(ST77XX_BLACK); // Clear the screen
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

void drawMenu() {
  // limpa a tela
  clearScreen();

  int buttonHeight = MENU_AREA_H / NUM_BUTTONS;
  for (int i = 0; i < NUM_BUTTONS; i++) {
    int buttonY = i * buttonHeight;
    bool isSelected = (i == selectedButton);

    // Draw button background
    tft.fillRect(0, buttonY, MENU_AREA_W, buttonHeight, isSelected ? COLOR_SELECTED : COLOR_NORMAL);

    // Draw button text
    tft.setCursor(10, buttonY + buttonHeight / 2 - 4); // Adjust for text alignment
    tft.setTextColor(ST7735_BLACK); // Text color
    tft.setTextSize(1); // Text size
    tft.print(menuItems[i]);

    displayImage("/character.bmp", CHARACTER_X, CHARACTER_Y);
  }
}

void drawInventory() {
  // limpa a tela
  clearScreen();
  // Obtém as informações do inventário
  Serial.println("entrou no drawInventory");
  const CharacterInfo& characterInfo = inf.getCharacterInfo();
  const std::vector<Inventory>& inventory = characterInfo.character.inventory;

  // Define a altura das seções do grid
  int cellWidth = tft.width() / inventory.size();

 // Mostra as informações do inventário na tela
  int xPos = 0;
  for (const Inventory& item : inventory) {

    // Carrega e exibe a imagem correspondente ao ID do item
    char filename[strlen(item.id.c_str()) + 6];
    strcpy(filename, "/");
    strcpy(filename, item.id.c_str());
    strcat(filename, ".bmp");
    displayImage(filename, xPos + (cellWidth - 18) / 2, 2);

    // Define a posição para a impressão do nome do item
    tft.setCursor(xPos + (cellWidth - strlen(item.name.c_str()) * 6) / 2, 22);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.print(item.name);

    // Define a posição para a impressão do texto do quantity
    tft.setCursor(xPos + (cellWidth - 20) / 2, 32);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2); // Tamanho de fonte maior para o quantity
    tft.println(item.quantity);

    // Atualiza a posição horizontal para a próxima célula do grid
    xPos += cellWidth; // Ajuste conforme necessário
  }

  delay(300);
}

void showSelectedMenu(int index) {
  if(index == 0) {
    drawInventory();
  }
}

void setup() {

  Serial.begin(115200);
  Serial.println(F("Hello! ST7735mini TFT"));
  pinMode(JOY_X_PIN, INPUT);
  pinMode(JOY_Y_PIN, INPUT);
  // pinMode(JOY_BTN_PIN, INPUT);
  
  pinMode(BTN_BACK_PIN, INPUT_PULLUP);
  pinMode(BTN_START_PIN, INPUT_PULLUP);

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
  pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(TFT_BACKLIGHT_PIN, HIGH);

  clearScreen();

  BLEDevice::init("ESP32_BLE");
  pServer = BLEDevice::createServer();
  pService = pServer->createService("1b9d0504-79f2-11ee-b962-0242ac120002");

  pCharacteristic = pService->createCharacteristic("e7ca6c9c-79f3-11ee-b962-0242ac120002", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new BLECallback());

  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("Waiting for a connection...");
  drawMenu();
  readJsonData();
}

void loop() {

  // Read joystick position
  int xVal = analogRead(JOY_X_PIN);
  int yVal = analogRead(JOY_Y_PIN);
  
  // Read joystick button
  // bool joyButton = digitalRead(JOY_BTN_PIN);

  // Read push buttons
  bool btn_back = !digitalRead(BTN_BACK_PIN);
  bool btn_start = !digitalRead(BTN_START_PIN);

  if (homeScreen) {
    if (yVal < 100) {
      selectedButton--; // Move up
      if (selectedButton < 0) selectedButton = NUM_BUTTONS - 1;
      drawMenu();
      delay(250); // Adjust delay for smooth navigation
    } else if (yVal > 4000) {
      selectedButton++; // Move down
      if (selectedButton >= NUM_BUTTONS) selectedButton = 0;
      drawMenu();
      delay(250); // Adjust delay for smooth navigation
    }
  }

  // Handle button press
  if (btn_start) {
    if (homeScreen) {
      // Clear the screen and show the selected menu
      showSelectedMenu(selectedButton);
      delay(500); // Adjust delay as needed
    } else {
      // Handle button 1 press logic for other screens
    }
  }

  if (btn_back) {
    if (!homeScreen) {
      // If not on menu screen, return to the menu screen
      homeScreen = true;
      clearScreen();
      drawMenu();
      delay(500); // Adjust delay as needed
    }
  }
}