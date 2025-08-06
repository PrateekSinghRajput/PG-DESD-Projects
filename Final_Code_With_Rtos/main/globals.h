#ifndef GLOBALS_H
#define GLOBALS_H

extern unsigned long lastCANMsgTime;
extern unsigned long lastNode1MsgTime;
extern unsigned long lastNode2MsgTime;

extern float lastTemp;
extern bool lastWater;
extern bool lastDoor;

extern float node3Latitude;
extern float node3Longitude;
extern float node2Load;

extern float webTemp;
extern bool webWaterEmpty;
extern bool webDoorOpen;
extern float webLatitude;
extern float webLongitude;
extern float webLoad;

extern bool node1Connected;
extern bool node2Connected;

extern const unsigned long CAN_TIMEOUT;

#endif
