#include <M5StickC.h>
#include "SHT3X.h"      //環境センサユニット ver.2用

//センサ用
#include <Wire.h>
#include "Adafruit_Sensor.h"
#include <Adafruit_BMP280.h>

//現在時間取得
#include <WiFi.h>
#include "time.h"

//Ambientに送信する
#include "Ambient.h"

const char* ssid     = "HUAWEI-F4E1";
const char* password = "64141340";

#define uS_TO_S_FACTOR 1000000  // マイクロ秒から秒への変換係数Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  1800       // ESP32がスリープ状態になる時間（秒単位） Time ESP32 will go to sleep (in seconds) */
//TIME_TO_SLEEP 600(10分)、1200（20分）,1800(30分)　3600（60分）

SHT3X sht30;                          //温湿度、環境センサユニット ver.2用
WiFiClient client;
Ambient ambient;


unsigned int channelId = 38726; // Ambient
const char* writeKey = "462e00822a97731e"; // ライトキー

float temp = 0.0;                         //温度変数
RTC_DATA_ATTR float aveTemp = 0.0;        //平均温度変数
RTC_DATA_ATTR float addTemp = 0.0;        //積算温度変数
RTC_DATA_ATTR char day = 0;               //日付管理変数
RTC_DATA_ATTR char flg = 1; 
char nowday = 0;

void wifi_conect(void);
void ambient_acosess(void);
void lcd_display(void);
void get_time(void);
char now[20];

void setup() 
{
    M5.begin(); 
    M5.Axp.ScreenBreath(10);                        //画面の輝度を少し下げる
    M5.Lcd.setRotation(3);                          //画面を("x*90°")回転させる
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(YELLOW);                    //LCDテキストの色を黄色に変更
    Wire.begin();                                   // I2Cを初期化する
}

void loop() 
{
      if(flg == 1)
      {
          //データを取得 
          if(sht30.get() == 0)
          {
            temp = sht30.cTemp;              //温度取り込み
          }
           
          wifi_conect();
          get_time();
                
          if((day != nowday) && (day!=0)) //dayが現在日付と異なる
          {
              day = nowday;
              addTemp = addTemp + aveTemp;
              
              ambient_acosess();
              
              aveTemp = 0;
          }
          else if(day == 0)
          {
               day = nowday;
          }
      
          if(aveTemp == 0)
          {
               aveTemp = temp; 
          }
          else
          {
               aveTemp = (temp+aveTemp)/2;
          }
          
          lcd_display();
          WiFi.disconnect();
  
          flg = 0;  
      }
      else
      {
          flg++;  
      }
      esp_deep_sleep(TIME_TO_SLEEP * uS_TO_S_FACTOR); //deepsleepに移行し、引数μ秒後に復帰する  

}


void wifi_conect(void)
{
    WiFi.begin(ssid, password);  //  Wi-Fi APに接続 
    while (WiFi.status() != WL_CONNECTED) //  Wi-Fi AP接続待ち
    {
       delay(500);
       Serial.print(".");
    }
    Serial.print("WiFi connected\r\nIP address: ");
    Serial.println(WiFi.localIP());
}

void get_time(void)
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
    
    nowday = timeInfo.tm_mday;
    
}

void ambient_acosess(void)
{
    //amibientにデータを送信する
    ambient.begin(channelId, writeKey, &client); // チャネルIDとライトキーを指定してAmbientの初期化

    // 積算温度、平均気温の値をAmbientに送信する 
    ambient.set(1, addTemp);
    ambient.set(2, aveTemp);
    
    ambient.send();
}
void lcd_display(void)
{
  
   //データをLCDに表示
   M5.Lcd.setTextSize(1);                        //テキストサイズを２に変更
   M5.Lcd.setTextColor(YELLOW,BLACK);                    //テキストの色を黄色に変更
   
   M5.Lcd.setCursor(10, 2);                        //LCDのカーソル位置変更
   M5.Lcd.printf("addTemp aveTemp Test\n");
   
   M5.Lcd.setCursor(10, 15);                             //LCDのカーソル位置変更
   M5.Lcd.print("Temp = ");     

   M5.Lcd.setCursor(80, 15);                             //LCDのカーソル位置変更
   M5.Lcd.printf("%4.1f",temp);
   
   M5.Lcd.setCursor(10, 28);
   M5.Lcd.print("Ave = ");
   M5.Lcd.setCursor(80, 28);                       //LCDのカーソル位置変更
   M5.Lcd.printf("%4.1f",aveTemp );

   M5.Lcd.setCursor(10, 42);
   M5.Lcd.printf("addition" );
   M5.Lcd.setCursor(80, 42);
   M5.Lcd.printf("%4.1f",addTemp );

   M5.Lcd.setCursor(10, 60);
   M5.Lcd.println(now);
}
