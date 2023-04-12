/*  _
** |_|___ ___
** | |_ -|_ -|
** |_|___|___|
**  iss(c)2020
**
**  public site: https://forum.defence-force.org/viewtopic.php?f=9&t=1699
**
**  Updated: 2020.10.05 - fixed bit operation - @john
**           2020.10.29 - fixed more bit operation - @thekorex
**           2022.10.12 - updated fillx() to accept custom 8-bit test patterns - Stan
**                      - modified first to fill the entire DRAM with the test pattern, then read back and verify (readx()) in a second pass - Stan
**                      - now stops after MAX_ERRORS count of errors, not after first error - Stan
**                      - speed/memory optimizations - Stan
**           2022.10.19 - enabled pullup on DRAM DO - Stan
**                      - added Auto-find 4164/41256. Jumper is no longer required, but if closed, will force a 4164 test - Stan
**                      - readAddress and writeAddress routines rewritten for direct port manipulation - significant speed increase - Stan
*/

/* ================================================================== */
#include <SoftwareSerial.h>

#define PORTC_DI        1  // PC1
#define PINB_DO         0  // PB0
#define PORTB_CAS       1  // PB1
#define PORTC_RAS       3  // PC3
#define PORTC_WE        2  // PC2

#define PORTC_XA0       4  // PC4
#define PORTD_XA1       2  // PD2
#define PORTC_XA2       5  // PC5
#define PORTD_XA3       6  // PD6
#define PORTD_XA4       5  // PD5
#define PORTD_XA5       4  // PD4
#define PORTD_XA6       7  // PD7
#define PORTD_XA7       3  // PD3
#define PORTC_XA8       0  // PC0

#define DI          15  // PC1
#define DO           8  // PB0
#define CAS          9  // PB1
#define RAS         17  // PC3
#define WE          16  // PC2

#define XA0         18  // PC4
#define XA1          2  // PD2
#define XA2         19  // PC5
#define XA3          6  // PD6
#define XA4          5  // PD5
#define XA5          4  // PD4
#define XA6          7  // PD7
#define XA7          3  // PD3
#define XA8         14  // PC0

#define M_TYPE      10  // PB2
#define R_LED       11  // PB3
#define G_LED       12  // PB4

#define RXD          0  // PD0
#define TXD          1  // PD1

#define BUS_SIZE     9

#define MAX_ERRORS   100

/* ================================================================== */
volatile word err_cnt;
volatile char  bus_size;


//SoftwareSerial USB(RXD, TXD);

const byte a_bus[BUS_SIZE] = {
  XA0, XA1, XA2, XA3, XA4, XA5, XA6, XA7, XA8
};

/*
void setBus(word a) {
  static char i;
  for (i = 0; i < BUS_SIZE; i++) {
    digitalWrite(a_bus[i], a & 1);
    a >>= 1;
  }
}
*/

void setBus(word a) {

  (a & 1) ? PORTC |= 1 << PORTC_XA0 : PORTC &= ~(1 << PORTC_XA0);
  (a & 2) ? PORTD |= 1 << PORTD_XA1 : PORTD &= ~(1 << PORTD_XA1);
  (a & 4) ? PORTC |= 1 << PORTC_XA2 : PORTC &= ~(1 << PORTC_XA2);
  (a & 8) ? PORTD |= 1 << PORTD_XA3 : PORTD &= ~(1 << PORTD_XA3);
  (a & 16) ? PORTD |= 1 << PORTD_XA4 : PORTD &= ~(1 << PORTD_XA4);
  (a & 32) ? PORTD |= 1 << PORTD_XA5 : PORTD &= ~(1 << PORTD_XA5);
  (a & 64) ? PORTD |= 1 << PORTD_XA6 : PORTD &= ~(1 << PORTD_XA6);
  (a & 128) ? PORTD |= 1 << PORTD_XA6 : PORTD &= ~(1 << PORTD_XA7);
  (a & 256) ? PORTC |= 1 << PORTC_XA8 : PORTC &= ~(1 << PORTC_XA8);  
    
}

void writeAddress(word r, word c, byte v) {

  /* row */
  setBus(r);
  PORTC &= ~(1 << PORTC_RAS);
  //digitalWrite(RAS, LOW);

  /* r/w */
  PORTC &= ~(1 << PORTC_WE);
  //digitalWrite(WE, LOW);

  /* bit write value */
  v &= 1;
  PORTC &= ~(1 << PORTC_DI);
  PORTC |= v << PORTC_DI;
  //digitalWrite(DI, (v & 1)? HIGH : LOW);

  /* column */
  setBus(c);
  PORTB &= ~(1 << PORTB_CAS);
  //digitalWrite(CAS, LOW);

  PORTC |= 1 << PORTC_RAS;
  PORTB |= 1 << PORTB_CAS;
  PORTC |= 1 << PORTC_WE;
  //digitalWrite(WE, HIGH);
  //digitalWrite(RAS, HIGH);
  //digitalWrite(CAS, HIGH);

}

byte readAddress(word r, word c) {
  static byte ret = 0;

  /* row */
  setBus(r);
  PORTC &= ~(1 << PORTC_RAS);
  //digitalWrite(RAS, LOW);

  /* column */
  setBus(c);
  PORTB &= ~(1 << PORTB_CAS);
  //digitalWrite(CAS, LOW);

  /* bit read value */
  //ret = digitalRead(DO);
  __asm__("nop\n\t"); //62.5ns
  __asm__("nop\n\t"); //62.5ns
  __asm__("nop\n\t"); //62.5ns
  __asm__("nop\n\t"); //62.5ns
  ret = PINB & (1 << PINB_DO); 

  PORTC |= 1 << PORTC_RAS;
  PORTB |= 1 << PORTB_CAS;
  //digitalWrite(RAS, HIGH);
  //digitalWrite(CAS, HIGH);

  return ret >>= PINB_DO;
}

byte error(int r, int c)
{
  unsigned long a = ((unsigned long)r << bus_size) + c;
  digitalWrite(R_LED, LOW);
  interrupts();
  Serial.print(" FAILED $");
  Serial.println(a, HEX);
  err_cnt += 1;
  Serial.flush();
  if (err_cnt >= MAX_ERRORS) 
    return 1;
  
  return 0;
}

void ok(void)
{
  byte i = 0;

  interrupts();
  Serial.print("Errors: "); Serial.println(err_cnt);

  if (err_cnt == 0) {
    digitalWrite(R_LED, HIGH);
    digitalWrite(G_LED, LOW);
    Serial.println("DRAM OK");
  } else {
    digitalWrite(R_LED, LOW);
    digitalWrite(G_LED, HIGH);
  }

  if (err_cnt >= MAX_ERRORS) 
    Serial.println(" TOO MANY ERRORS");

  for (i = 0; i < BUS_SIZE; i++)
    pinMode(a_bus[i], INPUT);

  pinMode(CAS, INPUT_PULLUP);
  pinMode(RAS, INPUT_PULLUP);
  pinMode(WE, INPUT_PULLUP);
  pinMode(DI, INPUT);
  Serial.flush();
}

void blink(void)
{
  digitalWrite(G_LED, LOW);
  digitalWrite(R_LED, LOW);
  delay(2000);
  digitalWrite(R_LED, HIGH);
  digitalWrite(G_LED, HIGH);
}

void green(char v) {
  digitalWrite(G_LED, v);
}

void fill(byte v) {
  word r, c, g = 0;
  word max_r = 1 << bus_size;
  word max_c = 1 << bus_size;
  v &= 1;

  if (err_cnt >= MAX_ERRORS) 
    return;

  for (r = 0; r < max_r; r++) {
    //green(g? HIGH : LOW);
    for (c = 0; c < max_c; c++) {
      writeAddress(r, c, v);
      if (readAddress(r, c) != v)
        if (error(r, c))
          return;
    }
    //g ^= 1;
  }
  blink();
}

void fillx(byte pattern) {
  word r, c = 0;
  byte v = 0;
  word max_r = 1 << bus_size;
  word max_c = 1 << bus_size;

  if (err_cnt >= MAX_ERRORS) 
    return;

  for (r = 0; r < max_r; r++) {
   // green(g? HIGH : LOW);
    for (c = 0; c < max_c; c++) {
      v = pattern & 1;
      writeAddress(r, c, v);
      pattern = (pattern >> 1) | (v << 7);
      //Serial.println(pattern, BIN);
    }
    //g ^= 1;
  }
  blink();
}

void readx(byte pattern) {
  word r, c = 0;
  byte v;
  word max_r = 1 << bus_size;
  word max_c = 1 << bus_size;

  if (err_cnt >= MAX_ERRORS) 
    return;

  for (r = 0; r < max_r; r++) {
    //green(g? HIGH : LOW);
    for (c = 0; c < max_c; c++) {
      v = pattern & 1;
      if (readAddress(r, c) != v)
        if (error(r, c))
          return;
      pattern = (pattern >> 1) | (v << 7); 
    }
    //g ^= 1;
  }
  blink();
}

void setup() {
  byte i;
  word test_addr = 0xF0;
  err_cnt = 0;

  pinMode(R_LED, OUTPUT);
  pinMode(G_LED, OUTPUT);
  digitalWrite(R_LED, LOW);
  digitalWrite(G_LED, LOW);

  Serial.begin(9600);
  while (!Serial)
    ; /* wait */

  Serial.println();
  Serial.print("DRAM TESTER ");

  for (i = 0; i < BUS_SIZE; i++)
    pinMode(a_bus[i], OUTPUT);
  
  digitalWrite(WE, HIGH);
  digitalWrite(RAS, HIGH);
  digitalWrite(CAS, HIGH);
  
  pinMode(CAS, OUTPUT);
  pinMode(RAS, OUTPUT);
  pinMode(WE, OUTPUT);
  pinMode(DI, OUTPUT);

  pinMode(M_TYPE, INPUT_PULLUP);
  pinMode(DO, INPUT_PULLUP);

  bus_size = BUS_SIZE - 1;
  if (digitalRead(M_TYPE)) {
    /* jumper not set - auto find 4164/41256 TODO: write/read more bits*/
    writeAddress(test_addr, test_addr, 0);
    writeAddress(test_addr | (1 << 8), test_addr | (1 << 8), 1);
    
    if (readAddress(test_addr, test_addr) == 0)
      bus_size = BUS_SIZE;
  } 

  if (bus_size == BUS_SIZE)
    Serial.println("256Kx1 ");
  else
    Serial.println("64Kx1 ");

  Serial.flush();

  noInterrupts();
  for (i = 0; i < 8; i++) {
    digitalWrite(RAS, LOW);
    digitalWrite(RAS, HIGH);
  }

  interrupts();
  digitalWrite(R_LED, HIGH);
  digitalWrite(G_LED, HIGH);
}

void loop() {

  interrupts(); Serial.println("0000 "); Serial.flush(); noInterrupts(); fill(0); 
  interrupts(); Serial.println("1111 "); Serial.flush(); noInterrupts(); fill(1);
  interrupts(); Serial.println("0101 "); Serial.flush(); noInterrupts(); fillx(0b10101010);readx(0b10101010);
  interrupts(); Serial.println("1010 "); Serial.flush(); noInterrupts(); fillx(0b01010101);readx(0b01010101);
  interrupts(); Serial.println("0011 "); Serial.flush(); noInterrupts(); fillx(0b11001100);readx(0b11001100);
  interrupts(); Serial.println("1100 "); Serial.flush(); noInterrupts(); fillx(0b00110011);readx(0b00110011);
  interrupts(); Serial.println("1001 "); Serial.flush(); noInterrupts(); fillx(0b10011001);readx(0b10011001);
  interrupts(); Serial.println("0110 "); Serial.flush(); noInterrupts(); fillx(0b01100110);readx(0b01100110);

  ok(); 
  while(1);  //TODO: wait for button push
}
