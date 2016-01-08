/*
俺の考えた最強の剣
by Daisuke IMAI <hine.gdw@gmail.com>
*/

#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <DFPlayer_Mini_Mp3.h>

// 各ラインを描画している時間(msec)
#define LINE_DELAY 20

// NeoPixel関連定義
#define NEOPIXEL_PIN 14
#define NEOPIXEL_NUM 80

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// SoftwareSerial関連定義(konashi、DFPlayerとの接続用)
#define TO_KONASHI_SERIAL_TX 7 // SoftwareSerialのTXは7番ピン
#define TO_KONASHI_SERIAL_RX 8 // SoftwareSerialのRXは8番ピン(LeonardではRXに使えるPINが限られていることに注意)
#define TO_DFPLAYER_SERIAL_TX 6 // SoftwareSerialのTXは6番ピン
#define TO_DFPLAYER_SERIAL_RX 5 // SoftwareSerialのRXは5番ピン(RXは使わない)

SoftwareSerial toKonashiSerial (TO_KONASHI_SERIAL_RX, TO_KONASHI_SERIAL_TX);
SoftwareSerial toDfplayerSerial (TO_DFPLAYER_SERIAL_RX, TO_DFPLAYER_SERIAL_TX);

// MPU-6050関連定義
#define MPU6050_ACCEL_XOUT_H       0x3B   // R
#define MPU6050_WHO_AM_I           0x75   // R
#define MPU6050_PWR_MGMT_1         0x6B   // R/W
#define MPU6050_I2C_ADDRESS 0x68

typedef union accel_t_gyro_union{
  struct{
    uint8_t x_accel_h;
    uint8_t x_accel_l;
    uint8_t y_accel_h;
    uint8_t y_accel_l;
    uint8_t z_accel_h;
    uint8_t z_accel_l;
    uint8_t t_h;
    uint8_t t_l;
    uint8_t x_gyro_h;
    uint8_t x_gyro_l;
    uint8_t y_gyro_h;
    uint8_t y_gyro_l;
    uint8_t z_gyro_h;
    uint8_t z_gyro_l;
  }
  reg;
  struct{
    int16_t x_accel;
    int16_t y_accel;
    int16_t z_accel;
    int16_t temperature;
    int16_t x_gyro;
    int16_t y_gyro;
    int16_t z_gyro;
  }
  value;
};

// konashiからの受信のためのグローバル変数
boolean receiving = false;
int received_count = 0;

// 剣の状態関連のグローバル変数
boolean vertical;
boolean reverse_direction;
float offset_x;

// カラーパレット
int color_data[16][3] = {
  {0x00, 0x00, 0x00},
  {0x33, 0x33, 0xcc},
  {0xff, 0x00, 0x00},
  {0xff, 0x99, 0xcc},
  {0xff, 0xcc, 0x99},
  {0x33, 0xcc, 0xcc},
  {0xff, 0xff, 0x00},
  {0xff, 0xff, 0xff},
  {0xff, 0xcc, 0x33},
  {0xcc, 0x99, 0x33},
  {0x33, 0x99, 0x99},
  {0x99, 0xcc, 0x99},
  {0x00, 0x00, 0x00},
  {0xcc, 0xff, 0x33},
  {0xff, 0xcc, 0x66},
  {0x00, 0xff, 0x00}
};

// 初期キャラクターデータ
int chara_data[16][16] = {
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0},
  {0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0, 0},
  {0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0},
  {0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0},
  {0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 0},
  {0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0},
  {0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0},
  {0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0},
  {0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0, 0},
  {0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// キャラクタ読み込みバッファ
int chara_buffer[16][16] = {
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// 初期化
void setup() {
  // デバグ用シリアル
  Serial.begin(115200);

  // konashi用シリアル
  pinMode(TO_KONASHI_SERIAL_RX, INPUT);
  pinMode(TO_KONASHI_SERIAL_TX, OUTPUT);
  toKonashiSerial.begin(9600);

  // DFPlayer用シリアル
  pinMode(TO_DFPLAYER_SERIAL_RX, INPUT);
  pinMode(TO_DFPLAYER_SERIAL_TX, OUTPUT);
  toDfplayerSerial.begin(9600);

  // SoftwareSerialの受信はkonashi用を選択
  toKonashiSerial.listen();

  // MPU-6050の初期化
  Wire.begin();
  int error;
  uint8_t c;
  Serial.print("InvenSense MPU-6050");
  Serial.println(" June 2012");
  error = MPU6050_read (MPU6050_WHO_AM_I, &c, 1);
  Serial.print("WHO_AM_I : ");
  Serial.print(c,HEX);
  Serial.print(", error = ");
  Serial.println(error,DEC);
  error = MPU6050_read (MPU6050_PWR_MGMT_1, &c, 1);
  Serial.print("PWR_MGMT_1 : ");
  Serial.print(c,HEX);
  Serial.print(", error = ");
  Serial.println(error,DEC);
  MPU6050_write_reg (MPU6050_PWR_MGMT_1, 0);

  delay(1500);

  // DFPlayerの初期化
  mp3_set_serial(toDfplayerSerial);      //set Serial for DFPlayer-mini mp3 module
  mp3_set_volume(30);
  delay(100);
  mp3_play(10);

  // とりあえず横持ちをベースとする
  vertical = false;
  reverse_direction = true;

  // NeoPixelの初期化
  pixels.begin();

  // 起動演出
  // 光が刃先に向けて伸びて行く
  for (int bar = 0; bar <= 30; bar++) {
    delay(150);
    pixels.setPixelColor(bar, pixels.Color(31, 31, 31));
    pixels.setPixelColor(59 - bar, pixels.Color(31, 31, 31));
    pixels.show();
  }
  // 全体が光り輝いて消える
  for (int bright = 0; bright < 10; bright++) {
    for (int bar = 0; bar <= 60; bar++) {
      pixels.setPixelColor(bar, pixels.Color(31 + bright * 10, 31 + bright * 10, 31 + bright * 10));
      pixels.setPixelColor(59 - bar, pixels.Color(31 + bright * 10, 31 + bright * 10, 31 + bright * 10));
    }
    pixels.show();
    delay(50);
  }
  delay(500);
  for (int bright = 0; bright < 10; bright++) {
    for (int bar = 0; bar <= 60; bar++) {
      pixels.setPixelColor(bar, pixels.Color(90 - 10 * bright, 90 - 10 * bright, 90 - 10 * bright));
      pixels.setPixelColor(59 - bar, pixels.Color(90 - 10 * bright, 90 - 10 * bright, 90 - 10 * bright));
    }
    pixels.show();
    delay(10);
  }
  // 柄を光らせる
  for (int i = 60; i < 80; i++) {
    pixels.setPixelColor(i, pixels.Color(127, 127, 0));
  }
  pixels.show();
}

void loop() {
  if (toKonashiSerial.available() > 0) {
    // konashi経由でデータを受信した場合の処理
    unsigned int received_data = toKonashiSerial.read();
    if (received_data == 255) {
      // ヘッダ
      received_count = 0;
      receiving = true;
    } else {
      // データ
      int data_x = received_count % 16;
      int data_y = (received_count - data_x) / 16;
      chara_buffer[data_y][data_x] = received_data;
      Serial.print(received_count);
      Serial.print(": ");
      Serial.println(received_data, HEX);
      if (received_count == 255) {
        // 全てを受信し終わった時の処理
        for (int chara_y = 0; chara_y < 16; chara_y++) {
          for (int chara_x = 0; chara_x < 16; chara_x++) {
            mp3_play(16); // 受信完了音
            chara_data[chara_y][chara_x] = chara_buffer[chara_y][chara_x];
            received_count = 0;
            receiving = false;
            /*
            for (int i = 60; i < 80; i++) {
              pixels.setPixelColor(i, pixels.Color(127, 127, 0));
            }
            pixels.show();
            */
          }
        }
      }
      received_count++;
    }
  }

  //MPU-6050の処理(データ受信中は行わない)
  if (!receiving) {
    // MPU-6050からの数値の取得
    int error;
    float dT;
    accel_t_gyro_union accel_t_gyro;
    error = MPU6050_read (MPU6050_ACCEL_XOUT_H, (uint8_t *) &accel_t_gyro, sizeof(accel_t_gyro));
    uint8_t swap;
#define SWAP(x,y) swap = x; x = y; y = swap
    SWAP (accel_t_gyro.reg.x_accel_h, accel_t_gyro.reg.x_accel_l);
    SWAP (accel_t_gyro.reg.y_accel_h, accel_t_gyro.reg.y_accel_l);
    SWAP (accel_t_gyro.reg.z_accel_h, accel_t_gyro.reg.z_accel_l);
    float acc_x = accel_t_gyro.value.x_accel / 16384.0; //FS_SEL_0 16,384 LSB / g
    float acc_y = accel_t_gyro.value.y_accel / 16384.0;
    float acc_z = accel_t_gyro.value.z_accel / 16384.0;

    // 縦横判定処理
    boolean reverse = false;
    if (vertical && (acc_y > -0.2)) {
      vertical = false;
      reverse = true;
      if (acc_x > 0) {
        offset_x = 1.0;
      } else {
        offset_x = -1.0;
      }
    } else if (!vertical && (acc_y < -0.8)) {
      vertical = true;
      reverse = false;
      offset_x = 0.0;
    }

    // 振っている方向の判定処理
    if (reverse_direction && (acc_x > offset_x + 0.5)) {
      reverse_direction = false;
      mp3_play(28); // 剣を振る音
      for (int line = 0; line < 16; line++) {
        draw_pattern(line, vertical, reverse);
        delay(LINE_DELAY);
      }
    } else if (!reverse_direction && (acc_x < offset_x - 0.5)) {
      reverse_direction = true;
      mp3_play(28); // 剣を振る音
      for (int line = 0; line < 16; line++) {
        draw_pattern(15 - line, vertical, reverse);
        delay(LINE_DELAY);
      }
    }
  }
}

// 剣両面の一斉描画
void draw_pattern(int line, boolean vertical, boolean reverse) {
  int x = 0;
  int y = 0;
  for (int pos = 0; pos < 15; pos++) {
    if (vertical) {
      // 剣をタテに持っている場合
      x = line;
      if (reverse) {
        // 剣の先が上
        y = pos + 1;
      } else {
        y = 14 - pos;
      }
    } else {
      // 剣をヨコに持っている場合
      if (reverse) {
        // 剣の先から書き始める
        x = 15 - pos;
      } else {
        x = pos + 1;
      }
      y = line;
    }
    pixels.setPixelColor(29 - pos * 2, pixels.Color(color_data[chara_data[y][x]][0], color_data[chara_data[y][x]][1], color_data[chara_data[y][x]][2]));
    pixels.setPixelColor(29 - pos * 2 + 1, pixels.Color(color_data[chara_data[y][x]][0], color_data[chara_data[y][x]][1], color_data[chara_data[y][x]][2]));
    pixels.setPixelColor(30 + pos * 2, pixels.Color(color_data[chara_data[y][x]][0], color_data[chara_data[y][x]][1], color_data[chara_data[y][x]][2]));
    pixels.setPixelColor(30 + pos * 2 - 1, pixels.Color(color_data[chara_data[y][x]][0], color_data[chara_data[y][x]][1], color_data[chara_data[y][x]][2]));
  }
  pixels.show();
}

// MPU6050_read
int MPU6050_read(int start, uint8_t *buffer, int size){
  int i, n, error;
  Wire.beginTransmission(MPU6050_I2C_ADDRESS);
  n = Wire.write(start);
  if (n != 1)
    return (-10);
  n = Wire.endTransmission(false);    // hold the I2C-bus
  if (n != 0)
    return (n);
  // Third parameter is true: relase I2C-bus after data is read.
  Wire.requestFrom(MPU6050_I2C_ADDRESS, size, true);
  i = 0;
  while(Wire.available() && i<size){
    buffer[i++]=Wire.read();
  }
  if ( i != size)
    return (-11);
  return (0);  // return : no error
}

// MPU6050_write
int MPU6050_write(int start, const uint8_t *pData, int size){
  int n, error;
  Wire.beginTransmission(MPU6050_I2C_ADDRESS);
  n = Wire.write(start);        // write the start address
  if (n != 1)
    return (-20);
  n = Wire.write(pData, size);  // write data bytes
  if (n != size)
    return (-21);
  error = Wire.endTransmission(true); // release the I2C-bus
  if (error != 0)
    return (error);
  return (0);         // return : no error
}

// MPU6050_write_reg
int MPU6050_write_reg(int reg, uint8_t data){
  int error;
  error = MPU6050_write(reg, &data, 1);
  return (error);
}
