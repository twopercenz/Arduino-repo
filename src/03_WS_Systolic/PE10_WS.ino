/*
 * ==========================================================================
 *  PE(1,0)  ―  WS(Weight Stationary) 시스톨릭 어레이
 * --------------------------------------------------------------------------
 *  이 PE 는 W[1][0] 을 가지고 있고,
 *  위쪽(PE00) 에서 오는 부분합 P_in 을 받아,
 *  그 사이클의 입력 A(=PE00 이 뿌린 값과 동일한 값 → 여기서는 위의 partial 자체가 A*W00 이므로
 *  실제 WS 구조에서는 A stream 이 아래로도 함께 전달됨을 반영해 A_stream 을 자체 배열로 관리)를
 *  통해 새 partial = A * W10 을 계산, P_in 과 더한 후
 *  우측(PE11) 로 흘려보낸다.
 *
 *  ※ 교과 실험용 단순화 : A stream 을 이 PE 에도 상수 배열로 저장해
 *    "위에서 흘러오는 A" 를 시뮬레이션한다 (전선을 줄이기 위한 실용적 절충).
 *    엄밀한 하드웨어 등가는 D7/D8 을 추가해 A 라인을 별도로 받는 것.
 *
 *  핀 배치
 *    D3  : CLOCK_P_IN   (← PE00)
 *    D5  : DATA_P_IN
 *    D2  : CLOCK_P_OUT  (→ PE11)   (P 를 우측으로 흘림)
 *    D4  : DATA_P_OUT
 *    A4/A5 : I2C LCD
 * ==========================================================================
 */
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- 핀 ----------
#define CLOCK_P_IN  3
#define DATA_P_IN   5
#define CLOCK_P_OUT 2
#define DATA_P_OUT  4

#define LCD_ADDR    0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ---------- 설정 ----------
#define TILE_MARK 254
#define K_SIZE    2

// 이 PE 가 담고 있는 가중치 (W[1][0])
const int W10 = 5;

// 위에서 함께 흘러오는 A 스트림 (실험 단순화용)
byte A_stream[] = { 1, 2 };
// 2x2x2: {1,2},  2x2x4: {1,2},  2x4x2: {1,2,3,4}

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

int receiveInt16(byte clkPin, byte dataPin) {
  byte hi = receiveByte(clkPin, dataPin);
  byte lo = receiveByte(clkPin, dataPin);
  return (int)((hi << 8) | lo);
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
  pinMode(CLOCK_P_IN,  INPUT);
  pinMode(DATA_P_IN,   INPUT);
  pinMode(CLOCK_P_OUT, OUTPUT);
  pinMode(DATA_P_OUT,  OUTPUT);
  digitalWrite(CLOCK_P_OUT, LOW);

  Serial.begin(9600);
  Wire.begin();
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("PE(1,0) WS Ready");
}

void loop() {
  static int k = 0;

  int p_in = receiveInt16(CLOCK_P_IN, DATA_P_IN);
  if ((p_in & 0xFFFF) == (int)((TILE_MARK << 8) | TILE_MARK)) {
    // 종료 전달
    sendInt16(CLOCK_P_OUT, DATA_P_OUT, (int)((TILE_MARK << 8) | TILE_MARK));
    Serial.println(F("PE(1,0) WS done"));
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("PE(1,0) WS");
    lcd.setCursor(0, 1); lcd.print("W10=");
    lcd.print(W10);
    k = 0;
    return;
  }

  byte a = A_stream[k];
  int  p_new = p_in + (int)a * W10;

  sendInt16(CLOCK_P_OUT, DATA_P_OUT, p_new);
  k++;
  if (k >= K_SIZE) k = 0;
}
