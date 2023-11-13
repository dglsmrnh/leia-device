#ifndef Information_h
#define Information_h

#include <Arduino.h>
#include <vector>

// Define a struct to represent an inventory item
struct Inventory {
  String id;
  String name;
  int quantity;
  String type;
};

// Define a struct to represent a quest
struct Quest {
  String id;
  String name;
  int status;
};

// Define a struct to represent an attribute
struct Attribute {
  String name;
  int points;
  String color;
};

// Define a struct to represent an image
struct Image {
  String type;
  String base64;
};

struct CharacterInfo {
  String username;
  int coins;
  std::vector<Inventory> inventoryList;
  std::vector<Quest> questsList;
  std::vector<Attribute> attributesList;
  std::vector<Image> imagesList;
};

class Information {
public:
  bool  processJson(const char* json, const bool saveJson);
  void addCharacterInfo(const CharacterInfo& characterInfo);
  const CharacterInfo& getCharacterInfo() const;
  const char* getCharacterInfoJson() const;
  bool saveImages() const;

private:
  CharacterInfo characterInfo;
};

#endif
