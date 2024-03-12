// #include <Adafruit_GFX.h>    // Core graphics library
// #include <Adafruit_ST7735.h> // Hardware-specific library for ST7735

// #define TFT_CS    5
// #define TFT_RST   4  // Or set to -1 and connect to Arduino RESET pin
// #define TFT_DC    15

// Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// #define JOY_X_PIN  2
// #define JOY_Y_PIN  0
// #define JOY_BTN_PIN 4

// #define BTN_PIN_1 26
// #define BTN_PIN_2 27

// // Define menu items
// const char* menuItems[] = {"Inventory", "Character", "Adventures"};
// const int numMenuItems = sizeof(menuItems) / sizeof(menuItems[0]);

// int menuIndex = 0;
// bool menuScreen = true; // Indicates whether the menu screen is being displayed

// void setup() {
//   Serial.begin(115200);
  
//   pinMode(JOY_X_PIN, INPUT);
//   pinMode(JOY_Y_PIN, INPUT);
//   pinMode(JOY_BTN_PIN, INPUT);
  
//   pinMode(BTN_PIN_1, INPUT);
//   pinMode(BTN_PIN_2, INPUT);

//   tft.initR(INITR_BLACKTAB); // Initialize display with black tab

//   drawMenu();
// }

// void loop() {
//   // Read joystick position
//   int xVal = analogRead(JOY_X_PIN);
//   int yVal = analogRead(JOY_Y_PIN);
  
//   // Read joystick button
//   bool joyButton = digitalRead(JOY_BTN_PIN);

//   // Read push buttons
//   bool btn1 = digitalRead(BTN_PIN_1);
//   bool btn2 = digitalRead(BTN_PIN_2);

//   // Handle joystick movement
//   if (menuScreen) {
//     if (yVal < 300) {
//       menuIndex--; // Move up
//       if (menuIndex < 0) menuIndex = numMenuItems - 1;
//       drawMenu();
//       delay(250); // Adjust delay for smooth navigation
//     } else if (yVal > 700) {
//       menuIndex++; // Move down
//       if (menuIndex >= numMenuItems) menuIndex = 0;
//       drawMenu();
//       delay(250); // Adjust delay for smooth navigation
//     }
//   }

//   // Handle button press
//   if (btn1) {
//     if (menuScreen) {
//       // Clear the screen and show the selected menu
//       clearScreen();
//       showSelectedMenu(menuIndex);
//       delay(500); // Adjust delay as needed
//     } else {
//       // Handle button 1 press logic for other screens
//     }
//   }

//   if (btn2) {
//     if (!menuScreen) {
//       // If not on menu screen, return to the menu screen
//       menuScreen = true;
//       clearScreen();
//       drawMenu();
//       delay(500); // Adjust delay as needed
//     }
//   }
// }

// void drawMenu() {
//   tft.fillScreen(ST7735_BLACK); // Clear the screen

//   // Draw menu items
//   for (int i = 0; i < numMenuItems; i++) {
//     if (i == menuIndex && menuScreen) {
//       tft.setTextColor(ST7735_WHITE, ST7735_BLACK); // Highlighted item
//     } else {
//       tft.setTextColor(ST7735_WHITE); // Regular item
//     }
//     tft.setCursor(0, i * 8); // Adjust spacing based on font size
//     tft.println(menuItems[i]);
//   }
  
//   if (menuScreen) {
//     // Draw "Voltar" option at the bottom
//     tft.setCursor(0, tft.height() - 8);
//     tft.setTextColor(ST7735_WHITE);
//     tft.println("Voltar");
//   }
// }

// void clearScreen() {
//   tft.fillScreen(ST7735_BLACK); // Clear the screen
// }

// void showSelectedMenu(int index) {
//   tft.setTextColor(ST7735_WHITE); // Set text color
//   tft.setCursor(0, 0); // Set cursor to top left corner
//   tft.println(menuItems[index]); // Print selected menu item
//   // Add logic to show content related to the selected menu item
// }
