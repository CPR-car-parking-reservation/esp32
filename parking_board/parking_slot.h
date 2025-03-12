#ifndef PARKING_SLOTS_H
#define PARKING_SLOTS_H

class Parking_slots {
public:
  String name;
  String floor;
  int pin;
  String old_value;
  int green;
  int red;
  int yellow;
};

//1 : idel
//0 : full
//2 : reserved

extern Parking_slots parking_slot[];
extern const int parking_size;

#endif