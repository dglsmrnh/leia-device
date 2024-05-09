#ifndef Information_h
#define Information_h

#include <Arduino.h>
#include <vector>

// Define a struct to represent an inventory item
struct Inventory {
  String id;
  String name;
  int quantity;
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

struct Character {
  String name;
  std::vector<Inventory> inventory;
  std::vector<Quest> quests;
  std::vector<Attribute> attributes;
  std::vector<Image> images;
  int coins;
  int level;
  String class_name;
  String race;
};

struct CharacterInfo {
  String username;
  Character character;  
};

class Information {
public:
  bool  processJson(const char* json, const bool saveJson);
  void addCharacterInfo(const CharacterInfo& characterInfo);
  void removePotion(const char* potion);
  bool saveCharacterInfoToSPIFFS();
  const CharacterInfo& getCharacterInfo() const;
  const char* getCharacterInfoJson() const;
  bool saveImages(const CharacterInfo& characterInfo) const;
  CharacterInfo characterInfo;
};

#endif
