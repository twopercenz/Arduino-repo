/*
 * ==========================================================================
 *  단일 보드 (폰 노이만 구조) - 행렬 곱셈
 * --------------------------------------------------------------------------
 *  하드웨어 : 아두이노 나노 1대 + I2C LCD (16x2)
 *  실험 목적 : CPU가 메모리에 순차 접근하며 A[i][k] * B[k][j] 를 계산하는
 *              전형적인 폰 노이만 구조의 처리 시간을 측정한다.
 *  독립변인  : 행렬 크기 (2x2x2, 2x2x4, 2x4x2)
 *  종속변인  : micros() 로 측정한 처리 시간 (us), 메모리 접근 횟수
 *
 *  ※ LCD 를 사용하지 않으려면 USE_LCD 를 0 으로 바꾸면 됩니다.
 * ==========================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- 설정 ----------
#define USE_LCD       1        // I2C LCD 사용 여부
#define LCD_ADDR   0x27        // I2C 주소 (모듈에 따라 0x3F 인 경우도 있음)

// 실험 대상 행렬 크기: (M x K) * (K x N) = (M x N)
// 아래 세 조합을 하나씩 바꿔가며 측정한다.
//   [1] 2x2x2  ->  M=2, K=2, N=2
//   [2] 2x2x4  ->  M=2, K=2, N=4
//   [3] 2x4x2  ->  M=2, K=4, N=2
#define M 2
#define K 2
#define N 2

#if USE_LCD
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
#endif

// ---------- 행렬 데이터 ----------
// 실험 재현성을 위해 값은 고정한다.
// (K, N 이 바뀔 때는 배열 크기와 초기값을 그에 맞게 수정)
int A[M][4] = {
  {1, 2, 3, 4},
  {5, 6, 7, 8}
};
int B[4][4] = {
  {1, 2, 3, 4},
  {5, 6, 7, 8},
  {9,10,11,12},
  {13,14,15,16}
};
long C[M][4];   // 결과 행렬 (최대 크기 확보)

// ---------- 통계 카운터 ----------
unsigned long memAccessCount = 0;   // 메모리 접근 횟수 (A, B, C 읽기/쓰기)
unsigned long mulAccCount    = 0;   // 곱셈-누산(MAC) 횟수

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }

#if USE_LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Von Neumann");
  lcd.setCursor(0, 1);
  lcd.print("Size:");
  lcd.print(M); lcd.print("x"); lcd.print(K); lcd.print("x"); lcd.print(N);
  delay(1500);
#endif

  Serial.println();
  Serial.println(F("=== Single Board (Von Neumann) ==="));
  Serial.print (F("Matrix size: "));
  Serial.print(M); Serial.print("x"); Serial.print(K);
  Serial.print(" * ");
  Serial.print(K); Serial.print("x"); Serial.println(N);

  // ---------- 시간 측정 시작 ----------
  unsigned long t_start = micros();

  // 표준 3중 for 문 : 순차 메모리 접근을 그대로 재현
  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      long sum = 0;
      memAccessCount++;                 // C[i][j] 초기화 접근
      for (int k = 0; k < K; k++) {
        int a = A[i][k];  memAccessCount++;  // A 읽기
        int b = B[k][j];  memAccessCount++;  // B 읽기
        sum += (long)a * (long)b;
        mulAccCount++;
      }
      C[i][j] = sum;
      memAccessCount++;                 // C 쓰기
    }
  }

  unsigned long t_end = micros();
  unsigned long elapsed = t_end - t_start;
  // ---------- 시간 측정 종료 ----------

  // ---------- 결과 출력 ----------
  Serial.println(F("--- Result C = A * B ---"));
  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      Serial.print(C[i][j]);
      Serial.print('\t');
    }
    Serial.println();
  }
  Serial.print(F("Elapsed (us) : ")); Serial.println(elapsed);
  Serial.print(F("Mem access   : ")); Serial.println(memAccessCount);
  Serial.print(F("MAC count    : ")); Serial.println(mulAccCount);

#if USE_LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T(us):");
  lcd.print(elapsed);
  lcd.setCursor(0, 1);
  lcd.print("Mem:");
  lcd.print(memAccessCount);
#endif
}

void loop() {
  // 1회 측정 후 종료 (필요 시 리셋 버튼으로 재측정)
}
