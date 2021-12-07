//温度測定プログラム
#include <M5StickC.h>
#include "SHT3X.h"   //M5Stack用環境センサユニット ver.2用
#include <Wire.h>    //M5Stack用環境センサユニット ver.2(I2C通信用)
#include "Adafruit_Sensor.h"//M5Stack用環境センサユニット ver.2
#include <Adafruit_BMP280.h>//M5Stack用環境センサユニット ver.2

SHT3X sht30;            //M5Stack用環境センサユニット ver.2用

#include <WiFi.h>       //wifi接続


#include "time.h"       //現在時間取得
#include "Ambient.h"    //Ambient

const char* ssid     = "HUAWEI-F4E1";//WIFI名
const char* password = "64141340";//wifiパスワード

WiFiClient client;
Ambient ambient;

unsigned int channelId = ambientID; // AmbientチャネルＩＤ
const char* writeKey = "ambientWriteKey"; // Ambientライトキー

#define uS_TO_S_FACTOR 1000000  // マイクロ秒から秒への変換係数Conversion factor for micro seconds to seconds
#define SLEEP_CONST  600        // スリープ時間の基準値

//関数プロトタイプ宣言
void wifi_conect(void);
void ambient_access(void);
void lcd_display(void);
void get_time(void);
void sleep_time_conf(void);
void led_blink(void);

//温度測定用変数
float temp = 0.0;                           //温度変数
RTC_DATA_ATTR float hAveTemp = 0;           //1時間平均気温変数
RTC_DATA_ATTR float aveTemp = 0.0;          //平均温度変数
RTC_DATA_ATTR float addTemp = 0.0;          //積算温度変数
RTC_DATA_ATTR float addTemp10 = 0.0;        //1時間平均積算温度変数
RTC_DATA_ATTR float aveTemp10 = 0.0;        //積算温度変数
float hum = 0.0;                            //湿度変数

//時間管理用変数
char now[20];
char now_day;
int now_hour;
int now_min;
int now_sec;
RTC_DATA_ATTR int old_hour = 0;
RTC_DATA_ATTR char day = 0;               //日付管理変数
int sleep_time;                           //deeepsleep時間
int elapsed_min;                          //経過分数
int elapsed_time;                         //経過時間

void setup() 
{
    M5.begin(); 
    M5.Axp.ScreenBreath(10);                        //画面の輝度を少し下げる
    M5.Lcd.setRotation(3);                          //画面を("x*90°")回転させる
    M5.Lcd.fillScreen(BLACK);                       //画面を黒で塗りつぶす
    Wire.begin();                                   //I2Cを初期化する
    pinMode(GPIO_NUM_10, OUTPUT);                   //内蔵led出力設定
}

void loop() 
{
  led_blink();  //led点滅
  if(sht30.get() == 0)
  {
    temp = sht30.cTemp;               //温度取り込み
    hum = sht30.humidity;             //湿度取り込み
  }
  
  if(hAveTemp == 0)//1時間平均気温取得用処理
  {
    hAveTemp = temp; 
  }
  else
  {
    hAveTemp = (hAveTemp+temp)/ 2;
  }
  wifi_conect();
  get_time();
  sleep_time_conf();
  //lcd_display();

  if(now_hour != old_hour)//前回起動時と現在時が違うとき
  {
    if(aveTemp == 0)    //平均気温が0の時＝初回起動時or日付が変わったとき
    {
      aveTemp = temp;
    }
    else
    {
      aveTemp = (temp + aveTemp)/2;
    }

    if(aveTemp10 == 0)  //平均気温が0の時＝初回起動時or日付が変わったとき
    {
      aveTemp10 = hAveTemp;
    }
    else
    {
      aveTemp10 = (hAveTemp + aveTemp10)/2;
    }
    
    hAveTemp = 0;      //1時間平均気温リセット 
  }
  
  if(day != now_day)    //日付が変わった時の処理
  {
    ambient_access();   //Ambientにデータを送信
    WiFi.disconnect();//wifi接続
    addTemp = addTemp + aveTemp;//積算気温計算
    aveTemp = 0;//日平均気温リセット

    addTemp10 = addTemp10 + aveTemp10;//
    aveTemp10 = 0;
  }
  else
  {
    WiFi.disconnect();  //wifi切断
  }

  day = now_day;
  old_hour = now_hour;
  esp_deep_sleep(sleep_time * uS_TO_S_FACTOR);
}


void wifi_conect(void)  //wifi接続関数
{
    WiFi.begin(ssid, password);           //  Wi-Fi APに接続

    while (WiFi.status() != WL_CONNECTED) //  Wi-Fi AP接続待ち
    {
       delay(500);
    }

    Serial.print("WiFi connected\r\nIP address: ");
    Serial.println(WiFi.localIP());
}


void get_time(void)     //時間取得関数
{
    configTime(9 * 3600, 0, "ntp.nict.jp"); // NTP時間をローカルに設定　/Set ntp time to local
    struct tm timeInfo;
    getLocalTime(&timeInfo); //ローカル時間を取得
  
    sprintf(now, "%04d/%02d/%02d %02d:%02d:%02d",
      timeInfo.tm_year + 1900,
      timeInfo.tm_mon + 1,
      timeInfo.tm_mday,
      timeInfo.tm_hour,
      timeInfo.tm_min,
      timeInfo.tm_sec
    );
    now_day = timeInfo.tm_mday;
    now_hour = timeInfo.tm_hour;
    now_min  = timeInfo.tm_min;
    now_sec  = timeInfo.tm_sec;
}


void sleep_time_conf(void)  //スリープ時間決定関数
{
  elapsed_min = (now_min%10)*60; //経過分＝現在分数÷10の剰余×60
  elapsed_time = now_sec + elapsed_min;//経過時間=現在秒数+経過分数
  sleep_time = SLEEP_CONST - elapsed_time;//スリープ時間＝睡眠定数-経過時間
}

void ambient_access(void)       //アンビエントアクセス関数
{
    ambient.begin(channelId, writeKey, &client); //チャネルIDとライトキーを指定してAmbientの初期化

    //Ambientに送信するデータをセット 
    ambient.set(1, aveTemp);
    ambient.set(2, addTemp);
    ambient.set(3,aveTemp10);
    ambient.set(4,addTemp10);
    
    ambient.send();                   //ambientにデータを送信
}

void led_blink(void)            //led点滅関数
{
  digitalWrite(GPIO_NUM_10, LOW);   //内蔵led点灯
  delay(500);
  digitalWrite(GPIO_NUM_10, HIGH);  //内蔵led消灯
}


void lcd_display(void)      //LCD表示関数
{
   M5.Lcd.setTextSize(1);                       //テキストサイズを1に変更
   M5.Lcd.setTextColor(YELLOW,BLACK);           //テキストの色を黄色に変更
   M5.Lcd.setCursor(20, 5);
   M5.Lcd.println(temp);                        //現在温度を表示
   M5.Lcd.setCursor(20, 20);
   M5.Lcd.println(aveTemp);                     //平均気温表示
   M5.Lcd.setCursor(20, 35);
   M5.Lcd.println(addTemp);                     //積算温度表示
   M5.Lcd.setCursor(20, 50);
   M5.Lcd.println(sleep_time);                  //スリープ時間を表示
   M5.Lcd.setCursor(10, 65);
   M5.Lcd.println(now);                         //現在時刻を表示
}