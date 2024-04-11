/*  
 *  LEIA app device
 */

// Required libraries
#include <esp_sleep.h>
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
#define JOY_BTN_PIN GPIO_NUM_25

#define BTN_BACK_PIN GPIO_NUM_33
#define BTN_START_PIN GPIO_NUM_32 

// BLR services
BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pCharacteristic;

bool deviceConnected = false;
int selectedInventoryIndex = 0; // Initialize with the first inventory item

// Image reader
SPIFFS_ImageReader reader;

Information inf;

#define NUM_BUTTONS 3 // Number of menu buttons
// Button colors
#define COLOR_NORMAL ST7735_WHITE
#define COLOR_SELECTED ST7735_BLUE

#define MENU_AREA_H 40
#define MENU_AREA_W 80
#define FOOTER_AREA 10

#define CHARACTER_X 80
#define CHARACTER_Y 5

#define JOYSTICK_UP 100
#define JOYSTICK_DOWN 4000
#define JOYSTICK_LEFT 100
#define JOYSTICK_RIGHT 4000

const char* menuItems[NUM_BUTTONS] = {"Inventario", "Sincronizar", "Desligar"};
int selectedButton = 0; // Index of the currently selected button
bool homeScreen = true; // Indicates whether the home screen is being displayed

// Define HP-related variables
#define MAX_HP 100
int hp = MAX_HP;
int hpDecreaseRate = 1; // HP decrease rate per HP_UPDATE_INTERVAL minutes (adjust as needed)
const char* hpFilePath = "/hp.txt";
const unsigned long HP_UPDATE_INTERVAL = 1 * 60 * 1000; // minutes in milliseconds

// Variable to store the last update time
RTC_DATA_ATTR unsigned long lastUpdateTime = millis();

void clearScreen() {
  tft.fillScreen(ST77XX_BLACK); // Clear the screen
}

// Function to put ESP32 to sleep
void goToSleep() {
  // Calculate time until next HP update
  unsigned long timeUntilUpdate = HP_UPDATE_INTERVAL - (millis() - lastUpdateTime);

  // Check if the time until next update is negative (should not happen, but just in case)
  if (timeUntilUpdate < 0) {
    timeUntilUpdate = 0;
  }

  // Set up wake-up time
  // esp_sleep_enable_timer_wakeup(timeUntilUpdate);

  // Enter deep sleep mode
  Serial.println("Entering deep sleep mode...");
  esp_deep_sleep_start();
}

class BLEServerCallback : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected.");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected.");
    }
};
std::string receivedValue;
class BLECallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    receivedValue += value;
    Serial.println("Received data: ");
    Serial.println(receivedValue.c_str());
    
    if(!inf.processJson(receivedValue.c_str(), true)) {
      Serial.println("JSON process error");
    }
    else {
      receivedValue = "";
    }
  }

  void onRead(BLECharacteristic* pCharacteristic) {
    pCharacteristic->setValue(inf.getCharacterInfoJson());
  }
};

void setupBLE() {
  BLEDevice::init("LEIA_BLE");
  BLEDevice::setMTU(512);
  pServer = BLEDevice::createServer();
  pService = pServer->createService("1b9d0504-79f2-11ee-b962-0242ac120002");

  pCharacteristic = pService->createCharacteristic("e7ca6c9c-79f3-11ee-b962-0242ac120002", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new BLECallback());

  pService->start();
}

void drawSynchronize() {
  clearScreen();
  // Clear the screen
  tft.fillScreen(ST77XX_BLACK);

  // Display BLE status
  if (BLEDevice::getInitialized()) {
    tft.setCursor(0, 10);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.println("Esperando conexao...");
    if (deviceConnected) {
      tft.println("Dispositivo conectado. Siga os passos no app.");
    } else {
      tft.println("Nenhum dispositivo conectado...");
    }
  } else {
    tft.setCursor(0, 10);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.println("BLE is not initialized");
  }
}

void startBLEAdvertising() {
  pServer->getAdvertising()->start();
  drawSynchronize();
  Serial.println("BLE advertising started. Waiting for a connection...");
}

void stopBLEAdvertising() {
  deviceConnected = false;
  pServer->getAdvertising()->stop();
  Serial.println("BLE advertising stopped.");
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
      Serial.println("process json error");
      return false;
    }
    return true;
  } else {
    Serial.println("Error opening file for reading");
    return false;
  }
}

void drawHP() {
  if(homeScreen) {
    tft.fillRect(0, tft.height() - 20, tft.width() - (CHARACTER_X + 20), 20, ST7735_BLACK); // Adjust dimensions as needed
    // Draw button text
    tft.setCursor(0, tft.height() - 20); // Adjust for text alignment
    tft.setTextColor(ST7735_WHITE); // Text color
    tft.setTextSize(1); // Text size
    tft.println("HP");
    tft.println(hp);
  }
}

void drawMenu() {
  // limpa a tela
  clearScreen();

  // zera variáveis usadas em outras telas
  selectedInventoryIndex = 0;

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
  }
  
  drawHP();
  displayImage("/character.bmp", CHARACTER_X, CHARACTER_Y);
}

void goToHomeScreen() {
  homeScreen = true;
  clearScreen();
  drawMenu();
}

// Load HP from SPIFFS
void loadHP() {
  Serial.println(hp);
  if (SPIFFS.exists(hpFilePath)) {
    File hpFile = SPIFFS.open(hpFilePath, FILE_READ);
    if (hpFile) {
      Serial.println(hpFile);
      String hpStr = hpFile.readStringUntil('\n'); // Read until newline character
      hp = hpStr.toInt(); // Convert string to integer
      Serial.println(hp);
      hpFile.close();
    } else {
      Serial.println("Failed to read HP from file");
    }
  }
}

// Function to save HP to SPIFFS
void saveHP() {
  File hpFile = SPIFFS.open(hpFilePath, FILE_WRITE);
  if (hpFile) {
    hpFile.println(hp);
    hpFile.close();
  } else {
    Serial.println("Failed to write HP to file");
  }
}

// Function to decrease HP
void decreaseHP() {
  loadHP();
  Serial.println(hp);

  // Calculate elapsed time in milliseconds
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - lastUpdateTime;

  // Calculate the number of intervals of 20 minutes elapsed
  unsigned long intervals = elapsedTime / HP_UPDATE_INTERVAL;
  Serial.println(intervals);

  // Calculate the HP decrease amount based on the number of intervals and the HP decrease rate
  int hpDecreaseAmount = intervals * hpDecreaseRate; // Decrease HP by hpDecreaseRate for each 20-minute interval
  Serial.println(hpDecreaseAmount);

  // Decrease HP by the calculated amount
  hp -= hpDecreaseAmount;
  Serial.println(hp);

  // Ensure HP doesn't go below 0
  if (hp < 0) {
    hp = 0;
  }

  saveHP();

  // Update last update time
  lastUpdateTime = currentTime;

  // Print updated HP
  Serial.print("HP: ");
  Serial.println(hp);

  drawHP();
}

// Function to recover HP based on the selected inventory item
void recoverHP() {
  // Load HP from SPIFFS
  loadHP();

  if(hp == MAX_HP) {
    return;
  }

  // Get inventory information
  const CharacterInfo& characterInfo = inf.getCharacterInfo();
  const std::vector<Inventory>& inventory = characterInfo.character.inventory;

  // Get the selected inventory item
  const Inventory& selectedPotion = inventory[selectedInventoryIndex];

  // Check the color of the potion and recover HP accordingly
  if (selectedPotion.name == "Amarela") {
    // Yellow potion: recovers 20% of lost HP
    int hpToRecover = (MAX_HP - hp) * 0.2; // 20% of lost HP
    hp += hpToRecover;
  } else if (selectedPotion.name == "Laranja") {
    // Orange potion: recovers 75% of lost HP
    int hpToRecover = (MAX_HP - hp) * 0.75; // 75% of lost HP
    hp += hpToRecover;
  } else if (selectedPotion.name == "Vermelha") {
    // Red potion: restores HP to maximum
    hp = MAX_HP;
  }

  // Ensure HP doesn't exceed maximum
  if (hp > MAX_HP) {
    hp = MAX_HP;
  }

  // Save HP to SPIFFS
  saveHP();

  loadHP();
  drawHP();

  inf.removePotion(selectedPotion.name.c_str());

  goToHomeScreen();
}

void drawInventory(int xVal) {
  // Clear the screen
  clearScreen();

  // Get inventory information
  const CharacterInfo& characterInfo = inf.getCharacterInfo();
  const std::vector<Inventory>& inventory = characterInfo.character.inventory;

  // Define the height of the grid sections
  int cellWidth = tft.width() / inventory.size();

  // Calculate the selected inventory index based on joystick input
  if (xVal < JOYSTICK_LEFT) {
    // If moving left, decrease the selected index (wrap around if necessary)
    selectedInventoryIndex = (selectedInventoryIndex == 0) ? inventory.size() - 1 : selectedInventoryIndex - 1;
  } else if (xVal > JOYSTICK_RIGHT) {
    // If moving right, increase the selected index (wrap around if necessary)
    selectedInventoryIndex = (selectedInventoryIndex + 1) % inventory.size();
  }

  // Show inventory information on the screen
  int xPos = 0;
  for (int i = 0; i < inventory.size(); ++i) {
    const Inventory& item = inventory[i];   

    // Check if the current tile is selected
    bool isSelected = (i == selectedInventoryIndex);

    // If this is the selected inventory item, paint the background white
    if (isSelected) {
      tft.fillRect(xPos, 0, cellWidth, tft.height(), ST77XX_WHITE);
    }

    // Load and display the image corresponding to the item ID
    char filename[strlen(item.id.c_str()) + 1];
    strcpy(filename, item.id.c_str());
    displayImage(filename, xPos + (cellWidth - 18) / 2, 2);

    // Set position for printing the item name
    tft.setCursor(xPos + (cellWidth - strlen(item.name.c_str()) * 6) / 2, 22);
    tft.setTextSize(1);
    if (isSelected) {
      tft.setTextColor(ST77XX_BLACK); // Invert text color if selected
    } else {
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.print(item.name);

    // Set position for printing the quantity text
    tft.setCursor(xPos + (cellWidth - 20) / 2, 32);
    tft.setTextSize(2); // Larger font size for quantity
    if (isSelected) {
      tft.setTextColor(ST77XX_BLACK); // Invert text color if selected
    } else {
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.println(item.quantity);

    // Update horizontal position for the next grid cell
    xPos += cellWidth; // Adjust as needed
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

  // Enable wake-up from the button press (low-to-high transition)
  esp_sleep_enable_ext0_wakeup(JOY_BTN_PIN, HIGH);

  // initialize SPIFFS
  if(!SPIFFS.begin()) {
    Serial.println("SPIFFS initialization failed!");
    while (1);
  }

  // initialize display and turn on backlight
  //tft.initR(INITR_MINI160x80_PLUGIN);  // Init ST7735S mini display (when seeing inversed)
  tft.initR(INITR_MINI160x80);  // Init ST7735S mini display
  tft.setRotation(1); // Set display rotation as needed
  Serial.println(F("Initialized"));
  delay(500);
  pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(TFT_BACKLIGHT_PIN, HIGH);

  clearScreen();

  setupBLE();

  readJsonData();
  loadHP();
  drawMenu();
}

void loop() {

  // Read joystick position
  int xVal = analogRead(JOY_X_PIN);
  int yVal = analogRead(JOY_Y_PIN);

  // Serial.print("x ");
  // Serial.println(xVal);
  // Serial.print("y ");
  // Serial.println(yVal);
  
  // Read joystick button
  // bool joyButton = digitalRead(JOY_BTN_PIN);

  // Read push buttons
  bool btn_back = !digitalRead(BTN_BACK_PIN);
  bool btn_start = !digitalRead(BTN_START_PIN);
  
  // Serial.print("back ");
  // Serial.println(btn_back);
  // Serial.print("start ");
  // Serial.println(btn_start);

  // Calculate time elapsed since last HP update
  unsigned long elapsedTime = millis() - lastUpdateTime;

  // Check if it's time to update HP
  if (elapsedTime >= HP_UPDATE_INTERVAL) {
    decreaseHP(); // Decrease HP if update interval has passed
  }

  if (homeScreen) { // in home screen
    if (yVal < JOYSTICK_UP) {
      selectedButton--; // Move up
      if (selectedButton < 0) selectedButton = NUM_BUTTONS - 1;
      drawMenu();
      
    } else if (yVal > JOYSTICK_DOWN) {
      selectedButton++; // Move down
      if (selectedButton >= NUM_BUTTONS) selectedButton = 0;
      drawMenu();
    }

    // Handle button press
    if (btn_start) {
      // Clear the screen and show the selected menu
      homeScreen = false;
      if(selectedButton == 0) { // Inventory
        drawInventory(xVal);
      }
      else if(selectedButton == 1) { // Syncronize
        startBLEAdvertising();
        drawSynchronize();
      }
      else if(selectedButton == 2) { // Sleep mode
        goToSleep();
      }
    }
  } else { //any menu screen
    if (btn_back) {
      // If not on menu screen, return to the menu screen
      if(selectedButton == 1) {
        stopBLEAdvertising();
      }
      goToHomeScreen();
    }
    else {
      if(btn_start) {
        if(selectedButton == 0) { // Inventory
          recoverHP();
        }
        else if(selectedButton == 1) { // Syncronize
        }
        else if(selectedButton == 2) { // Sleep mode

        }
      }
      else {
        if(selectedButton == 0) { // Inventory
          if (xVal < JOYSTICK_LEFT) {
            // If moving left, redraw inventory
            drawInventory(xVal);
          } else if (xVal > JOYSTICK_RIGHT) {
            // If moving right, redraw inventory
            drawInventory(xVal);
          }
        }
        else if(selectedButton == 1) { // Syncronize
          
        }
        else if(selectedButton == 2) { // Sleep mode
          goToSleep();
        }
      }
    }
  }

  delay(300); // Adjust delay for smooth navigation
}