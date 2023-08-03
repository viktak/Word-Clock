#ifndef STRUCTS_H
#define STRUCTS_H


struct BirthDay{
    int day;
    int month;
};

struct config{
  char ssid[32];
  char password[32];

  char friendlyName[30];
  unsigned int heartbeatInterval;

  signed char timeZone;

  char mqttServer[64];
  int mqttPort;
  char mqttTopic[32];

  long mode;

  BirthDay birthDays[2];

};


struct MyAnimationState{
    RgbColor StartingColor;
    RgbColor EndingColor;
};

struct segment{
    uint16_t id;
    RgbColor color;
    unsigned int positions[12];
    unsigned int length;
    bool fadeDirection;
    bool isActive;
    MyAnimationState animationState;
};




#endif