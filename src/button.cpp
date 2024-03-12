// #include <Adafruit_GFX.h>
// #include <Adafruit_ST7735.h>

// #define TFT_CS    5
// #define TFT_RST   4
// #define TFT_DC    15

// Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// #define NUM_BUTTONS 3 // Number of menu buttons

// const char* menuItems[NUM_BUTTONS] = {"Option 1", "Option 2", "Option 3"};
// int selectedButton = 0; // Index of the currently selected button

// // Button colors
// #define COLOR_NORMAL ST7735_WHITE
// #define COLOR_SELECTED ST7735_BLUE

// void setup() {
//   tft.initR(INITR_BLACKTAB); // Initialize display with black tab
//   tft.fillScreen(ST7735_BLACK); // Clear the screen

//   // Draw menu buttons
//   drawMenu();
// }

// void loop() {
//   // Your loop logic here
// }

// void drawMenu() {
//   int buttonHeight = tft.height() / NUM_BUTTONS;
//   for (int i = 0; i < NUM_BUTTONS; i++) {
//     int buttonY = i * buttonHeight;
//     bool isSelected = (i == selectedButton);

//     // Draw button background
//     tft.fillRect(0, buttonY, tft.width(), buttonHeight, isSelected ? COLOR_SELECTED : COLOR_NORMAL);

//     // Draw button text
//     tft.setCursor(10, buttonY + buttonHeight / 2 - 4); // Adjust for text alignment
//     tft.setTextColor(ST7735_BLACK); // Text color
//     tft.setTextSize(1); // Text size
//     tft.print(menuItems[i]);
//   }
// }
