/*
 * ==========================================================================
 *  PE(0,0)  ―  WS(Weight Stationary) 시스톨릭 어레이
 * --------------------------------------------------------------------------
 *  개념 : 가중치 행렬 W 의 각 원소는 해당 위치의 PE 에 코드 상수로 고정한다.
 *         입력 행렬 X 의 원소만 격자 위에서 흘려보내며 부분합을 계산한다.
 *         (일반적 표기 : Y = X * W ,  여기서는 X 를 A 로, W 를 B 로 취급)
 *
 *  이 PE 는 W[0][0] 을 가지고 있고,
 *  입력 A 를 오른쪽(PE01)으로,
 *  부분합 P 를 아래쪽(PE10)으로 흘려보낸다.
 *
 *  핀 배치
 *    D2  : CLOCK_A_OUT   (→ PE01 로 A 데이터용 클럭)
 *    D4  : DATA_A_OUT    (→ PE01 로 A 데이터)
 *    D3  : CLOCK_P_OUT   (→ PE10 로 부분합 P 클럭)
 *    D5  : DATA_P_OUT    (→ PE10 로 부분합 P 데이터, 16bit)
 *    D6  : TIMING_TRIG
 *    A4/A5 : I2C LCD
 * ==========================================================================
 */
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- 핀 ----------
#define CLOCK_A_OUT 2
#define DATA_A_OUT  4
#define CLOCK_P_OUT 3
#define DATA_P_OUT  5
#define TIMING_TRIG 6

#define LCD_ADDR    0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ---------- 설정 ----------
#define TILE_MARK  254
#define K_SIZE     2

// 이 PE 가 담고 있는 가중치 (상수로 고정)
const int W00 = 1;

// 입력 A 스트림 (여기서는 A[0][k])
byte A_stream[] = { 1, 2 };
// 2x2x2 : {1,2},  2x2x4 : {1,2},  2x4x2 : {1,2,3,4}

const int CLK_HALF_US = 20;

// ---------- 통신 ----------
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

// 16bit 부분합 전송 (상위 8bit → 하위 8bit)
void sendInt16(byte clkPin, byte dataPin, int value) {
  sendByte(clkPin, dataPin, (byte)((value >> 8) & 0xFF));
  sendByte(clkPin, dataPin, (byte)( value       & 0xFF));
}

void setup() {
  pinMode(CLOCK_A_OUT, OUTPUT);
  pinMode(DATA_A_OUT,  OUTPUT);
  pinMode(CLOCK_P_OUT, OUTPUT);
  pinMode(DATA_P_OUT,  OUTPUT);
  pinMode(TIMING_TRIG, OUTPUT);

  digitalWrite(CLOCK_A_OUT, LOW);
  digitalWrite(CLOCK_P_OUT, LOW);
  digitalWrite(TIMING_TRIG, LOW);

  Serial.begin(9600);
  Wire.begin();
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("PE(0,0) WS Ready");

  delay(3000);   // 다른 보드 초기화 대기

  // ---------- 타이밍 트리거 ----------
  digitalWrite(TIMING_TRIG, HIGH);

  // ---------- WS 시스톨릭 스트리밍 ----------
  // 매 사이클마다 A[k] 를 받아 (여기서는 자체 배열),
  //   1) partial = A[k] * W00 계산
  //   2) partial 을 아래(PE10) 로 전달
  //   3) A[k] 를 오른쪽(PE01) 으로 전달
  for (int k = 0; k < K_SIZE; k++) {
    byte a = A_stream[k];
    int  p = (int)a * W00;

    sendInt16(CLOCK_P_OUT, DATA_P_OUT, p);   // 부분합 → 아래
    sendByte (CLOCK_A_OUT, DATA_A_OUT, a);   // A     → 우측
  }

  // ---------- 종료 구분자 ----------
  sendByte(CLOCK_A_OUT, DATA_A_OUT, TILE_MARK);
  sendInt16(CLOCK_P_OUT, DATA_P_OUT, (int)((TILE_MARK << 8) | TILE_MARK));  // 0xFEFE

  Serial.println(F("PE(0,0) WS done"));
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("PE(0,0) WS");
  lcd.setCursor(0, 1); lcd.print("W00=");
  lcd.print(W00);
}

void loop() {}
