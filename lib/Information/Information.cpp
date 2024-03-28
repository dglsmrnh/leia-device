#include "Information.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Base64.h>

bool Information::processJson(const char* json, const bool saveJson) {
    // Parse the JSON content
    DynamicJsonDocument doc(100000);
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.println("Error parsing JSON");
        return false;
    }

    CharacterInfo characterInfo;
    // Process character information
    Serial.println(doc["data"]["username"].as<String>());
    characterInfo.username = doc["username"].as<String>();
    Serial.println(characterInfo.username);

    JsonObject character = doc["character"];
    characterInfo.character.name = character["name"].as<String>();
    characterInfo.character.coins = character["coins"].as<int>();
    characterInfo.character.class_name = character["class_name"].as<String>();
    characterInfo.character.race = character["race"].as<String>();

    // Process attributes
    JsonArray attributesArray = character["attributes"];
    for (JsonObject attribute : attributesArray) {
        Attribute attributeItem;
        attributeItem.name = attribute["name"].as<String>();
        attributeItem.points = attribute["points"].as<int>();
        attributeItem.color = attribute["color"].as<String>();
        characterInfo.character.attributes.push_back(attributeItem);
    }

    JsonArray inventoryArray = character["inventory"];
    for (JsonObject item : inventoryArray) {
        Inventory inventoryItem;
        inventoryItem.id = item["id"].as<String>();
        inventoryItem.name = item["name"].as<String>();
        inventoryItem.quantity = item["quantity"].as<int>();
        characterInfo.character.inventory.push_back(inventoryItem);
    }

    JsonArray questsArray = character["quests"];
    for (JsonObject item : questsArray) {
        Quest questItem;
        questItem.id = item["id"].as<String>();
        questItem.name = item["name"].as<String>();
        questItem.status = item["status"].as<int>();
        characterInfo.character.quests.push_back(questItem);
    }

    addCharacterInfo(characterInfo);
    // saveImages();

    if(saveJson) {
        // Save the binary data to a file in SPIFFS
        // Open the JSON file for writing
        File file = SPIFFS.open("/data.json", "w");

        // Check if the file was opened successfully
        if (file) {
            // Serialize the JSON object and write to the file
            serializeJson(doc, file);

            // Close the file
            file.close();

            Serial.println("JSON successfully saved to SPIFFS!");
        } else {
            Serial.println("Error opening the file for writing!");
            return false;
        }
    }

    return true;
}

void Information::addCharacterInfo(const CharacterInfo& characterInfo) {
    Serial.println("save character info");
    this->characterInfo = characterInfo;
}

const CharacterInfo& Information::getCharacterInfo() const {
    Serial.println(this->characterInfo.username);
    return this->characterInfo;
}

const char* Information::getCharacterInfoJson() const {
    // Create a JSON document to store the character information
    DynamicJsonDocument doc(100000);

    // Set the username
    doc["username"] = this->characterInfo.username;

    // Create a nested object for the character information
    JsonObject characterObject = doc.createNestedObject("character");
    characterObject["coins"] = this->characterInfo.character.coins;

    // Create an array for the inventory items
    JsonArray inventoryArray = characterObject.createNestedArray("inventory");

    // Add each inventory item to the array
    for (const Inventory& inventoryItem : this->characterInfo.character.inventory) {
        JsonObject itemObject = inventoryArray.createNestedObject();
        itemObject["id"] = inventoryItem.id;
        itemObject["name"] = inventoryItem.name;
        itemObject["quantity"] = inventoryItem.quantity;
    }

    // Convert the JSON document to a string
    String jsonString;
    serializeJson(doc, jsonString);

    return jsonString.c_str();
}

bool Information::saveImages() const {
  for (const Image& image : this->characterInfo.character.images) {
    if (!image.type.isEmpty() && !image.base64.isEmpty()) {
        char* base64code = new char[strlen(image.base64.c_str()) + 1];
        strcpy(base64code, image.base64.c_str());
        // Decode the base64 data to binary data
        int inputStringLength = strlen(base64code);
        
        int decodedLength = Base64.decodedLength(base64code, inputStringLength);
    
        // Use a fixed-size buffer for decoding
        char decodedData[decodedLength + 1];
    
        // Decode directly to the buffer
        Base64.decode(decodedData, base64code, inputStringLength);
        decodedData[decodedLength] = '\0'; // Null-terminate the string
    
        Serial.print("Decoded string is:\t");
        Serial.println(decodedData);
    
        // Save the binary data to a file in SPIFFS
        String filePath = "/";
        filePath.concat(image.type);
        filePath.concat(".bmp");
        File file = SPIFFS.open(filePath.c_str(), "w");
        if (file) {
            // Write directly from the buffer
            file.write((uint8_t*)decodedData, decodedLength);
            file.close();
            Serial.print("Image saved to SPIFFS as: ");
            Serial.println(filePath);
        } else {
            Serial.println("Failed to open file for writing.");
            return false;
        }
    }
  }

  return true;
}

bool Information::saveCharacterInfoToSPIFFS() {
  // Create a JSON document to hold the character info
  DynamicJsonDocument doc(100000);

  // Add username to the document
  doc["username"] = this->characterInfo.username;

  // Create a JsonObject for the character and add its properties
  JsonObject character = doc.createNestedObject("character");
  character["name"] = this->characterInfo.character.name;
  character["coins"] = this->characterInfo.character.coins;
  character["class_name"] = this->characterInfo.character.class_name;
  character["race"] = this->characterInfo.character.race;

  // Add attributes array to the character
  JsonArray attributesArray = character.createNestedArray("attributes");
  for (const Attribute& attribute : this->characterInfo.character.attributes) {
    JsonObject attr = attributesArray.createNestedObject();
    attr["name"] = attribute.name;
    attr["points"] = attribute.points;
    attr["color"] = attribute.color;
  }

  // Add inventory array to the character
  JsonArray inventoryArray = character.createNestedArray("inventory");
  for (const Inventory& item : this->characterInfo.character.inventory) {
    JsonObject inv = inventoryArray.createNestedObject();
    inv["id"] = item.id;
    inv["name"] = item.name;
    inv["quantity"] = item.quantity;
  }

  // Add quests array to the character
  JsonArray questsArray = character.createNestedArray("quests");
  for (const Quest& quest : this->characterInfo.character.quests) {
    JsonObject q = questsArray.createNestedObject();
    q["id"] = quest.id;
    q["name"] = quest.name;
    q["status"] = quest.status;
  }

  // Open the data.json file in write mode
  File file = SPIFFS.open("/data.json", "w");
  if (!file) {
    Serial.println("Failed to open data.json file for writing");
    return false;
  }

  // Serialize the JSON document and write it to the file
  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write JSON to file");
    file.close();
    return false;
  }

  // Close the file
  file.close();

  Serial.println("Character info successfully saved to SPIFFS!");
  return true;
}

void Information::removePotion(const char* potion) {

  // Loop through the inventory to find the potion by name
  for (Inventory& item : this->characterInfo.character.inventory) {
    const char* itemName = item.name.c_str();
    if (strcmp(itemName, potion) == 0) {
      // Decrement the quantity of the potion by 1
      if (item.quantity > 0) {
        item.quantity--;
      }
      break; // Stop after updating the potion
    }
  }

  saveCharacterInfoToSPIFFS();
}
