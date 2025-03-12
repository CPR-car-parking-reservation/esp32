// A1
#define a1_ir_sensor 32
#define a1_led_green 21
#define a1_led_yellow 22
#define a1_led_red 23

// A2
#define a2_ir_sensor 33
#define a2_led_green 17
#define a2_led_yellow 18
#define a2_led_red 19

// A3
#define a3_ir_sensor 34
#define a3_led_green 13
#define a3_led_yellow 14
#define a3_led_red 15

// B1
#define b1_ir_sensor 35
#define b1_led_green 5
#define b1_led_yellow 16
#define b1_led_red 3

// B2
#define b2_ir_sensor 36
#define b2_led_green 25
#define b2_led_yellow 26
#define b2_led_red 27

// B3
#define b3_ir_sensor 39
#define b3_led_green 0
#define b3_led_yellow 2
#define b3_led_red 4


void setup_sensor_led() {
  pinMode(a1_ir_sensor, INPUT);
  pinMode(a1_led_green, OUTPUT);
  pinMode(a1_led_yellow, OUTPUT);
  pinMode(a1_led_red, OUTPUT);

  pinMode(a2_ir_sensor, INPUT);
  pinMode(a2_led_green, OUTPUT);
  pinMode(a2_led_yellow, OUTPUT);
  pinMode(a2_led_red, OUTPUT);

  pinMode(a3_ir_sensor, INPUT);
  pinMode(a3_led_green, OUTPUT);
  pinMode(a3_led_yellow, OUTPUT);
  pinMode(a3_led_red, OUTPUT);

  pinMode(b1_ir_sensor, INPUT);
  pinMode(b1_led_green, OUTPUT);
  pinMode(b1_led_yellow, OUTPUT);
  pinMode(b1_led_red, OUTPUT);

  pinMode(b2_ir_sensor, INPUT);
  pinMode(b2_led_green, OUTPUT);
  pinMode(b2_led_yellow, OUTPUT);
  pinMode(b2_led_red, OUTPUT);

  pinMode(b3_ir_sensor, INPUT);
  pinMode(b3_led_green, OUTPUT);
  pinMode(b3_led_yellow, OUTPUT);
  pinMode(b3_led_red, OUTPUT);
}