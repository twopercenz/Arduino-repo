
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- 핀 ----------
#define CLOCK_A_OUT 2 // A 타이밍 맞추는 핀
#define DATA_A_OUT 4 // A 데이터 보내는 핀

#define CLOCK_P_OUT 3 // P 타이밍 맞추는 핀
#define DATA_P_OUT 5 // P 데이터 보내는 핀

#define TIME_CHECK_OUT 6

#define LCD_ADDR 0x27

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
