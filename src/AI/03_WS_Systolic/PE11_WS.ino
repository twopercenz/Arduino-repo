/*
 * ==========================================================================
 *  PE(1,1)  ―  WS(Weight Stationary) 시스톨릭 어레이  [최종 노드]
 * --------------------------------------------------------------------------
 *  이 PE 는 W[1][1] 을 가지고 있고,
 *  좌측(PE10) 에서 오는 부분합 P_in 을 받아 자신의 W11 로 마지막 누산을
 *  수행한 뒤 결과를 LCD 에 출력한다.
 *  또한 PE(0,0) 의 D6(TIMING_TRIG) HIGH 를 감지해 micros() 시작,
 *  타일 구분자 수신 시 micros() 종료.
 *
 *  핀 배치
 *    D2  : CLOCK_P_IN   (← PE10)
 *    D4  : DATA_P_IN
 *    D3  : CLOCK_P_TOP  (← PE01, 위에서 오는 부분합)
 *    D5  : DATA_P_TOP
 *    D6  : TIMING_TRIG_IN
 *    A4/A5 : I2C LCD
 * ==========================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- 핀 ----------
#define CLOCK_P_IN  2
#define DATA_P_IN   4
#define CLOCK_P_TOP 3
#define DATA_P_TOP  5
#define TIMING_TRIG_IN 6

#define LCD_ADDR    0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ---------- 설정 ----------
#define TILE_MARK 254
#define K_SIZE    2

// 이 PE 가 담고 있는 가중치
const int W11 = 6;

// 이 PE 도 A 스트림을 참조 (실험 단순화)
byte A_stream[] = { 1, 2 };

const int CLK_HALF_US = 20;

// ---------- 상태 ----------
long C00 = 0, C01 = 0, C10 = 0, C11 = 0;
unsigned long t_start = 0, t_end = 0;
bool timing_started = false;
bool left_done = false, top_done = false;

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

void setup() {
  pinMode(CLOCK_P_IN,  INPUT);
  pinMode(DATA_P_IN,   INPUT);
  pinMode(CLOCK_P_TOP, INPUT);
  pinMode(DATA_P_TOP,  INPUT);
  pinMode(TIMING_TRIG_IN, INPUT);

  Serial.begin(9600);
  Wire.begin();
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("PE(1,1) WS Ready");
}

void loop() {
  // 1) 타이밍 시작
  if (!timing_started && digitalRead(TIMING_TRIG_IN) == HIGH) {
    t_start = micros();
    timing_started = true;
  }

  static int k = 0;
  const int END_MARK16 = (int)((TILE_MARK << 8) | TILE_MARK);

  // 좌측 (P10) 에서 부분합 수신 → C10 완성값
  if (!left_done) {
    int p_left = receiveInt16(CLOCK_P_IN, DATA_P_IN);
    if ((p_left & 0xFFFF) == END_MARK16) {
      left_done = true;
    } else {
      // p_left 는 PE10 에서 이미 A*W00 + A*W10 (부분합) 을 마친 값
      // 여기서 이 PE 의 W11 로 마지막 곱을 더한다.
      byte a = A_stream[k];
      C11 += (long)p_left + (long)a * W11;
      // C10 은 PE10 이 위에서 그대로 흘려보낸 A*W00+A*W10 그 자체 → 참고용
      C10 += (long)p_left;
      k++;
      if (k >= K_SIZE) k = 0;
    }
  }

  // 상단 (P01) 에서 부분합 수신 → C01
  if (!top_done) {
    int p_top = receiveInt16(CLOCK_P_TOP, DATA_P_TOP);
    if ((p_top & 0xFFFF) == END_MARK16) {
      top_done = true;
    } else {
      C01 += (long)p_top;
    }
  }

  // 3) 종료
  if (left_done && top_done) {
    t_end = micros();
    unsigned long elapsed = t_end - t_start;

    Serial.println(F("=== WS Systolic Result ==="));
    Serial.print(F("C01(from top) = ")); Serial.println(C01);
    Serial.print(F("C10(from left) = ")); Serial.println(C10);
    Serial.print(F("C11 = ")); Serial.println(C11);
    Serial.print(F("Elapsed (us) = ")); Serial.println(elapsed);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("C11=");
    lcd.print(C11);
    lcd.setCursor(0, 1);
    lcd.print("T(us):");
    lcd.print(elapsed);

    while (true) { ; }
  }
}
