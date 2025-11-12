
// SP8T Pins:
// ENABLE
// LS
// V3
// V2
// V1

#define ENABLE    2
#define LS        3
#define V3        4
#define V2        5
#define V1        6 


#define v1_msk 0x1 // 0x1 = 0b 0001
#define v2_msk 0x2 // 0x2 = 0b 0010
#define v3_msk 0x4 // 0x4 = 0b 0100

void setup() {
  pinMode(ENABLE, OUTPUT);
  pinMode(LS, OUTPUT);
  pinMode(V1, OUTPUT);
  pinMode(V2, OUTPUT);
  pinMode(V3, OUTPUT);

  // Port options: 1-8
  enablePort(8);
}

// Open specified port and close the rest
void enablePort(uint8_t port) {
  digitalWrite(ENABLE, LOW);
  digitalWrite(LS, LOW);

  // port 1 = 0b 0000 0000
  uint8_t port_mux = port - 1;

  uint8_t v1_val = port_mux & v1_msk; 
  uint8_t v2_val = port_mux & v2_msk; 
  uint8_t v3_val = port_mux & v3_msk; 

  digitalWrite(V1, v1_val);
  digitalWrite(V2, v2_val);
  digitalWrite(V3, v3_val);
}

void loop() { /* Pass */ }
