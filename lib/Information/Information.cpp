#include "Information.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Base64.h>

bool Information::processJson(const char* json, const bool saveJson) {
    // Parse the JSON content
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.println("Error parsing JSON");
        return false;
    }

    CharacterInfo characterInfo;
    // Process character information
    characterInfo.username = doc["username"].as<String>();

    JsonObject character = doc["character"];
    characterInfo.coins = character["coins"];

    // Process attributes
    JsonArray attributesArray = character["attributes"];
    for (JsonObject attribute : attributesArray) {
        Attribute attributeItem;
        attributeItem.name = attribute["name"].as<String>();
        attributeItem.points = attribute["points"].as<int>();
        attributeItem.color = attribute["color"].as<String>();
        characterInfo.attributesList.push_back(attributeItem);
    }

    JsonArray inventoryArray = character["inventory"];
    for (JsonObject item : inventoryArray) {
        Inventory inventoryItem;
        inventoryItem.id = item["id"].as<String>();
        inventoryItem.name = item["name"].as<String>();
        inventoryItem.quantity = item["quantity"].as<int>();
        inventoryItem.type = item["type"].as<String>();
        characterInfo.inventoryList.push_back(inventoryItem);
    }

    JsonArray questsArray = character["quests"];
    for (JsonObject item : questsArray) {
        Quest questItem;
        questItem.id = item["id"].as<String>();
        questItem.name = item["name"].as<String>();
        questItem.status = item["status"].as<int>();
        characterInfo.questsList.push_back(questItem);
    }

    addCharacterInfo(characterInfo);

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
    this->characterInfo = characterInfo;
}

const CharacterInfo& Information::getCharacterInfo() const {
    return this->characterInfo;
}

const char* Information::getCharacterInfoJson() const {
    // Create a JSON document to store the character information
    DynamicJsonDocument doc(2048);

    // Set the username
    doc["username"] = this->characterInfo.username;

    // Create a nested object for the character information
    JsonObject characterObject = doc.createNestedObject("character");
    characterObject["coins"] = this->characterInfo.coins;

    // Create an array for the inventory items
    JsonArray inventoryArray = characterObject.createNestedArray("inventory");

    // Add each inventory item to the array
    for (const Inventory& inventoryItem : this->characterInfo.inventoryList) {
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
  for (const Image& image : this->characterInfo.imagesList) {
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
