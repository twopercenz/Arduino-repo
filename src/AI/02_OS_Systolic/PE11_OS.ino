/*
 * ==========================================================================
 *  PE(1,1)  ―  OS(Output Stationary) 시스톨릭 어레이  [최종 노드]
 * --------------------------------------------------------------------------
 *  역할 : 좌측(PE10)으로부터 A, 위쪽(PE01)으로부터 B 를 받아
 *         C[1][1] 부분합을 누적한다.
 *         또한 PE(0,0) 의 D6(TIMING_TRIG) HIGH 를 감지하여 micros() 시작,
 *         두 채널의 TILE_MARK 를 모두 받으면 micros() 종료 후 LCD 표시.
 *
 *  핀 배치
 *    D2  : CLOCK_A_IN   (← PE10)
 *    D4  : DATA_A_IN
 *    D3  : CLOCK_B_IN   (← PE01)
 *    D5  : DATA_B_IN
 *    D6  : TIMING_TRIG_IN  (← PE00 의 D6 와 병렬 연결)
 *    A4/A5 : I2C LCD
 * ==========================================================================
 */
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- 핀 ----------
#define CLOCK_A_IN 2
#define DATA_A_IN  4
#define CLOCK_B_IN 3
#define DATA_B_IN  5
#define TIMING_TRIG_IN 6

#define LCD_ADDR   0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ---------- 설정 ----------
#define TILE_MARK 254
#define K_SIZE    2

// ---------- 상태 ----------
long localSum = 0;   // C[1][1]
unsigned long t_start = 0, t_end = 0;
bool timing_started = false;
bool a_done = false, b_done = false;

byte receiveByte(byte clkPin, byte dataPin) {
  byte value = 0;
  for (int i = 0; i < 8; i++) {
    while (digitalRead(clkPin) == LOW) { ; }
    value = (value << 1) | (digitalRead(dataPin) & 0x01);
    while (digitalRead(clkPin) == HIGH) { ; }
  }
  return value;
}

void setup() {
  pinMode(CLOCK_A_IN, INPUT);
  pinMode(DATA_A_IN,  INPUT);
  pinMode(CLOCK_B_IN, INPUT);
  pinMode(DATA_B_IN,  INPUT);
  pinMode(TIMING_TRIG_IN, INPUT);

  Serial.begin(9600);
  Wire.begin();
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("PE(1,1) OS Ready");
}

void loop() {
  // 1) 타이밍 트리거 감지 (한 번만)
  if (!timing_started && digitalRead(TIMING_TRIG_IN) == HIGH) {
    t_start = micros();
    timing_started = true;
  }

  // 2) A / B 두 채널을 번갈아 처리
  //    (실험 편의를 위해 순차 폴링. 병렬 처리를 하려면 인터럽트 사용)
  static int k_idx = 0;

  // A 수신 - 짝지어 처리하기 위해 A 먼저
  if (!a_done) {
    byte a = receiveByte(CLOCK_A_IN, DATA_A_IN);
    if (a == TILE_MARK) { a_done = true; }
    else {
      // B 수신 (같은 사이클)
      byte b = receiveByte(CLOCK_B_IN, DATA_B_IN);
      if (b == TILE_MARK) { b_done = true; }
      else {
        localSum += (long)a * (long)b;
        k_idx++;
      }
    }
  } else if (!b_done) {
    // A 는 끝났는데 B 만 남은 경우 (이론적으로 동시 종료지만 안전장치)
    byte b = receiveByte(CLOCK_B_IN, DATA_B_IN);
    if (b == TILE_MARK) b_done = true;
  }

  // 3) 양쪽 채널 모두 종료 → 시간 확정 및 표시
  if (a_done && b_done) {
    t_end = micros();
    unsigned long elapsed = t_end - t_start;

    Serial.println(F("=== OS Systolic Result ==="));
    Serial.print(F("C11 = ")); Serial.println(localSum);
    Serial.print(F("Elapsed (us) = ")); Serial.println(elapsed);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("C11=");
    lcd.print(localSum);
    lcd.setCursor(0, 1);
    lcd.print("T(us):");
    lcd.print(elapsed);

    // 무한 대기
    while (true) { ; }
  }
}
