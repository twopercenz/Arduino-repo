/*
 * ==========================================================================
 *  PE(0,1)  ―  OS(Output Stationary) 시스톨릭 어레이
 * --------------------------------------------------------------------------
 *  역할 : PE(0,0) 에서 오는 A 를 수신하고, 자신은 B[k][1] 을 공급.
 *         C[0][1] 부분합을 누적하고, A 를 그대로 우측(없음)이 아닌 아래(PE11)로 전달.
 *         ※ 2x2 격자에서 (0,1) 은 우상단이므로 A 는 자기 자리에서 종료되고,
 *           B 는 아래쪽 PE(1,1) 로 계속 흘려보낸다.
 *
 *  핀 배치
 *    D2  : CLOCK_A_IN   (← PE00 로부터 A 클럭 수신)
 *    D4  : DATA_A_IN    (← PE00 로부터 A 데이터 수신)
 *    D3  : CLOCK_B_OUT  (→ PE11 로 B 클럭 출력)
 *    D5  : DATA_B_OUT   (→ PE11 로 B 데이터 출력)
 *    D6  : (사용 안 함, 미연결)
 *    A4/A5 : I2C LCD
 * ==========================================================================
 */
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- 핀 ----------
#define CLOCK_A_IN  2
#define DATA_A_IN   4
#define CLOCK_B_OUT 3
#define DATA_B_OUT  5

#define LCD_ADDR    0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ---------- 설정 ----------
#define TILE_MARK  254
#define K_SIZE     2

// 이 PE 가 담당하는 B 열 : B[k][1]
byte B_col1[] = { 2, 6 };
// (참고)
// 2x2x2 : { 2, 6 }
// 2x2x4 : { 2, 6 }   (열 1)
// 2x4x2 : { 2, 6, 10, 14 }

const int CLK_HALF_US = 20;

// ---------- 함수 ----------
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
long localSum = 0;    // C[0][1]

void setup() {
  pinMode(CLOCK_A_IN,  INPUT);
  pinMode(DATA_A_IN,   INPUT);
  pinMode(CLOCK_B_OUT, OUTPUT);
  pinMode(DATA_B_OUT,  OUTPUT);
  digitalWrite(CLOCK_B_OUT, LOW);

  Serial.begin(9600);
  Wire.begin();
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("PE(0,1) OS Ready");
}

void loop() {
  // A 수신
  byte a = receiveByte(CLOCK_A_IN, DATA_A_IN);
  if (a == TILE_MARK) {
    // 타일 종료 : 결과 확정
    Serial.print(F("PE(0,1) C01 = "));
    Serial.println(localSum);
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("PE(0,1) OS");
    lcd.setCursor(0, 1); lcd.print("C01=");
    lcd.print(localSum);

    // 아래 PE 에도 종료 신호 전달
    sendByte(CLOCK_B_OUT, DATA_B_OUT, TILE_MARK);

    localSum = 0;
    return;
  }

  // 현재 열 인덱스는 수신 순서로 결정 (0..K_SIZE-1)
  static int k = 0;

  // 자신의 B[k][1] 을 A와 곱해 누적
  byte b = B_col1[k];
  localSum += (long)a * (long)b;

  // B 는 아래쪽(PE11) 로 흘려보낸다
  sendByte(CLOCK_B_OUT, DATA_B_OUT, b);

  k++;
  if (k >= K_SIZE) k = 0;
}
