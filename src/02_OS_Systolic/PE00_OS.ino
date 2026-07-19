/*
 * ==========================================================================
 *  PE(0,0)  ―  OS(Output Stationary) 시스톨릭 어레이
 * --------------------------------------------------------------------------
 *  역할 : 데이터 공급자(Feeder). A 행렬 원소는 오른쪽(PE01)으로,
 *         B 행렬 원소는 아래쪽(PE10)으로 흘려보낸다.
 *         자신은 A[0][k], B[k][0] 을 받아 C[0][0] 부분합을 누적한다.
 *
 *  핀 배치 (아두이노 나노 기준)
 *    D2  : CLOCK_A_OUT   (오른쪽 PE01 로 A 데이터용 클럭 출력)
 *    D4  : DATA_A_OUT    (오른쪽 PE01 로 A 데이터 출력)
 *    D3  : CLOCK_B_OUT   (아래쪽 PE10 로 B 데이터용 클럭 출력)
 *    D5  : DATA_B_OUT    (아래쪽 PE10 로 B 데이터 출력)
 *    D6  : TIMING_TRIG   (연산 시작 트리거 - PE11 이 감지)
 *    A4/A5 : I2C LCD
 * ==========================================================================
 */
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- 핀 정의 ----------
#define CLOCK_A_OUT 2
#define DATA_A_OUT  4
#define CLOCK_B_OUT 3
#define DATA_B_OUT  5
#define TIMING_TRIG 6

#define LCD_ADDR    0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ---------- 실험 파라미터 ----------
// (M x K) * (K x N) 중 이 PE 는 C[0][0] 담당
#define K_SIZE 2       // 2x2x2 실험 (K=2). 2x2x4 는 K=2, 2x4x2 는 K=4 로 변경
#define TILE_MARK 254  // 타일 구분자

// ---------- 데이터 (2x4x2 실험은 A[0]={1,2,3,4}, B에서 열 0 = {1,5,9,13}) ----------
// 예시: 2x2x2
byte A_row0[] = { 1, 2 };          // A[0][0], A[0][1]
byte B_col0[] = { 1, 5 };          // B[0][0], B[1][0]

// (참고) 다른 크기 실험 시 아래를 활성화해서 사용하세요.
// byte A_row0[] = { 1, 2 };        byte B_col0[] = { 1, 5 };         // 2x2x4
// byte A_row0[] = { 1, 2, 3, 4 };  byte B_col0[] = { 1, 5, 9, 13 };  // 2x4x2

// ---------- 통신 지연 ----------
// 클럭 스큐(옆 PE 가 안정적으로 읽을 수 있도록 준 시간)
const int CLK_HALF_US = 20;

// ---------- 커스텀 송신 함수 ----------
void sendByte(byte clkPin, byte dataPin, byte value) {
  // MSB First
  for (int i = 7; i >= 0; i--) {
    digitalWrite(clkPin, LOW);
    digitalWrite(dataPin, (value >> i) & 0x01);
    delayMicroseconds(CLK_HALF_US);
    digitalWrite(clkPin, HIGH);
    delayMicroseconds(CLK_HALF_US);
  }
  digitalWrite(clkPin, LOW);
}

// ---------- 로컬 누산 ----------
long localSum = 0;   // C[0][0]

void setup() {
  pinMode(CLOCK_A_OUT, OUTPUT);
  pinMode(DATA_A_OUT,  OUTPUT);
  pinMode(CLOCK_B_OUT, OUTPUT);
  pinMode(DATA_B_OUT,  OUTPUT);
  pinMode(TIMING_TRIG, OUTPUT);

  digitalWrite(CLOCK_A_OUT, LOW);
  digitalWrite(CLOCK_B_OUT, LOW);
  digitalWrite(TIMING_TRIG, LOW);

  Serial.begin(9600);
  Wire.begin();
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("PE(0,0) OS Ready");

  delay(3000);   // 다른 보드가 초기화 될 때까지 대기

  // ---------- 연산 시작 신호 ----------
  digitalWrite(TIMING_TRIG, HIGH);   // PE11 이 이 신호로 micros() 시작

  // ---------- 데이터 스트리밍 ----------
  // 시스톨릭 스킴 : 매 사이클마다 A 행 원소를 우측으로,
  //                  B 열 원소를 아래로 동시에 흘린다.
  for (int k = 0; k < K_SIZE; k++) {
    byte a = A_row0[k];
    byte b = B_col0[k];

    // 자기 자신의 누산 (C[0][0] += A[0][k] * B[k][0])
    localSum += (long)a * (long)b;

    // 오른쪽 (PE01) 으로 A 전송
    sendByte(CLOCK_A_OUT, DATA_A_OUT, a);
    // 아래쪽 (PE10) 으로 B 전송
    sendByte(CLOCK_B_OUT, DATA_B_OUT, b);
  }

  // ---------- 타일 종료 구분자 ----------
  sendByte(CLOCK_A_OUT, DATA_A_OUT, TILE_MARK);
  sendByte(CLOCK_B_OUT, DATA_B_OUT, TILE_MARK);

  // ---------- 결과 출력 ----------
  Serial.print(F("PE(0,0) C00 = "));
  Serial.println(localSum);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("PE(0,0) OS");
  lcd.setCursor(0, 1); lcd.print("C00=");
  lcd.print(localSum);
}

void loop() {
  // idle
}
