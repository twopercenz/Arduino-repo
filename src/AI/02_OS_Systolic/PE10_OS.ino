/*
 * ==========================================================================
 *  PE(1,0)  ―  OS(Output Stationary) 시스톨릭 어레이
 * --------------------------------------------------------------------------
 *  역할 : PE(0,0) 에서 오는 B 를 수신하고, 자신은 A[1][k] 을 공급.
 *         C[1][0] 부분합을 누적하고, B 를 그대로 아래(없음)로가 아니라
 *         A 는 우측(PE11) 로 계속 흘려보낸다.
 *
 *  핀 배치
 *    D3  : CLOCK_B_IN   (← PE00 로부터 B 클럭 수신)
 *    D5  : DATA_B_IN    (← PE00 로부터 B 데이터 수신)
 *    D2  : CLOCK_A_OUT  (→ PE11 로 A 클럭 출력)
 *    D4  : DATA_A_OUT   (→ PE11 로 A 데이터 출력)
 *    A4/A5 : I2C LCD
 * ==========================================================================
 */
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- 핀 ----------
#define CLOCK_B_IN  3
#define DATA_B_IN   5
#define CLOCK_A_OUT 2
#define DATA_A_OUT  4

#define LCD_ADDR    0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ---------- 설정 ----------
#define TILE_MARK 254
#define K_SIZE    2

// 이 PE 가 담당하는 A 행 : A[1][k]
byte A_row1[] = { 5, 6 };
// (참고) 2x2x2 : {5, 6}
//        2x2x4 : {5, 6}
//        2x4x2 : {5, 6, 7, 8}

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

// ---------- 상태 ----------
long localSum = 0;   // C[1][0]

void setup() {
  pinMode(CLOCK_B_IN,  INPUT);
  pinMode(DATA_B_IN,   INPUT);
  pinMode(CLOCK_A_OUT, OUTPUT);
  pinMode(DATA_A_OUT,  OUTPUT);
  digitalWrite(CLOCK_A_OUT, LOW);

  Serial.begin(9600);
  Wire.begin();
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("PE(1,0) OS Ready");
}

void loop() {
  byte b = receiveByte(CLOCK_B_IN, DATA_B_IN);
  if (b == TILE_MARK) {
    Serial.print(F("PE(1,0) C10 = "));
    Serial.println(localSum);
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("PE(1,0) OS");
    lcd.setCursor(0, 1); lcd.print("C10=");
    lcd.print(localSum);

    sendByte(CLOCK_A_OUT, DATA_A_OUT, TILE_MARK);
    localSum = 0;
    return;
  }

  static int k = 0;
  byte a = A_row1[k];
  localSum += (long)a * (long)b;

  // A 는 우측(PE11) 로 흘려보낸다
  sendByte(CLOCK_A_OUT, DATA_A_OUT, a);

  k++;
  if (k >= K_SIZE) k = 0;
}
