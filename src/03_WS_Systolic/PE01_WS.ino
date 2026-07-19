/*
 * ==========================================================================
 *  PE(0,1)  ―  WS(Weight Stationary) 시스톨릭 어레이
 * --------------------------------------------------------------------------
 *  이 PE 는 W[0][1] 을 가지고 있고,
 *  좌측(PE00) 으로부터 A 를 받아 partial = A * W01 을 계산한 뒤
 *  아래쪽(PE11) 로 부분합 P 를 흘려보낸다.
 *  A 는 우측이 없으므로 자기 자리에서 종료된다.
 *
 *  핀 배치
 *    D2  : CLOCK_A_IN   (← PE00)
 *    D4  : DATA_A_IN
 *    D3  : CLOCK_P_OUT  (→ PE11)
 *    D5  : DATA_P_OUT
 *    A4/A5 : I2C LCD
 * ==========================================================================
 */
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- 핀 ----------
#define CLOCK_A_IN  2
#define DATA_A_IN   4
#define CLOCK_P_OUT 3
#define DATA_P_OUT  5

#define LCD_ADDR    0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ---------- 설정 ----------
#define TILE_MARK 254
#define K_SIZE    2

// 이 PE 가 담고 있는 가중치
const int W01 = 2;

const int CLK_HALF_US = 20;

// ---------- 통신 ----------
byte receiveByte(byte clkPin, byte dataPin) {
  byte value = 0;
  for (int i = 0; i < 8; i++) {
    while (digitalRead(clkPin) == LOW) { ; }
    value = (value << 1) | (digitalRead(dataPin) & 0x01);
    while (digitalRead(clkPin) == HIGH) { ; }
  }
  return value;
}

void sendByte(byte clkPin, byte dataPin, byte value) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(clkPin, LOW);
    digitalWrite(dataPin, (value >> i) & 0x01);
    delayMicroseconds(CLK_HALF_US);
    digitalWrite(clkPin, HIGH);
    delayMicroseconds(CLK_HALF_US);
  }
  digitalWrite(clkPin, LOW);
}

void sendInt16(byte clkPin, byte dataPin, int value) {
  sendByte(clkPin, dataPin, (byte)((value >> 8) & 0xFF));
  sendByte(clkPin, dataPin, (byte)( value       & 0xFF));
}

void setup() {
  pinMode(CLOCK_A_IN,  INPUT);
  pinMode(DATA_A_IN,   INPUT);
  pinMode(CLOCK_P_OUT, OUTPUT);
  pinMode(DATA_P_OUT,  OUTPUT);
  digitalWrite(CLOCK_P_OUT, LOW);

  Serial.begin(9600);
  Wire.begin();
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("PE(0,1) WS Ready");
}

void loop() {
  byte a = receiveByte(CLOCK_A_IN, DATA_A_IN);
  if (a == TILE_MARK) {
    // 종료 구분자 전달
    sendInt16(CLOCK_P_OUT, DATA_P_OUT, (int)((TILE_MARK << 8) | TILE_MARK));
    Serial.println(F("PE(0,1) WS done"));
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("PE(0,1) WS");
    lcd.setCursor(0, 1); lcd.print("W01=");
    lcd.print(W01);
    return;
  }

  int p = (int)a * W01;
  sendInt16(CLOCK_P_OUT, DATA_P_OUT, p);   // 부분합 아래로
}
