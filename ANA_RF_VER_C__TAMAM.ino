//KÜTÜPHANELER.

#include "nRF24L01.h"
#include "RF24.h"
#include <SD.h> //Load SD card library
#include<SPI.h> //Load SPI Library
#include<Wire.h>
#include<Math.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>
#include <Adafruit_BMP280.h>



#define BMP_SCK  (13)  // Kullanmadığı halde Bmp280 için gerekli olan tanımlamalar.
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

Adafruit_BMP280 bmp; // BMP 280 İçin nesne tanımlama
TinyGPS gps; // GPS İçin nesne tanımlama

const int MPU_addr = 0x68; //MPU adresi

SoftwareSerial ss(3, 4); //rx tx   GPS İçin fazladan tanımlanan rx ve tx pinleri.

RF24 verici(9, 10);

double ang_x, ang_y , ang_z , derece_x, derece_y, deger_y, deger_x;// Açı değerleri İçin tanımlanan değişkenler.
int birinci_ayrilma = 0, ikinci_ayrilma = 0;

unsigned long yeni_zaman, eski_zaman = 0; //millis zaman fonksiyonu kullanımı için değişkenler.
int m = 0;
double irtifa_deger, irtifa, eski_irtifa, onceki_irtifa; //Bmp 280 basınç sensöründen irtifa değeri okumak için tanımlanan değişkenler.

double irtifa_d[4] = {}; //farklı irtifa değerleri için dizi tanımlama
// Açı değerleri İçin tanımlanan diziler
float x_dizi[7] = {};
float y_dizi[7] = {};

int chipSelect = 10; //SD kartın CS pini için tanımlanan D0 pini
File mySensorData; //SD kart İçin oluşturulan nesne.

float data[7];
const byte adres[6] = "00001"; //Adres tanımlaması
int durum = 0;

//*************************************************************************************************************************************************************************************

void setup() {
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(9, OUTPUT);

  SPI.begin();
  verici.begin(); //Haberleşmeyi başlat
  verici.openWritingPipe(adres); //Veri gönderilecek adres
  verici.stopListening();//Modülü verici olarak tanımlıyoruz



  Serial.begin(9600);//Seri haberlşeme başlatılır
  ss.begin(9600); // GPS İçin SPI başlatma
  Wire.begin();   // İvme sensörü İçin I2C Başlatılır
  bmp.begin();   // BMP 280 başlatma
  SD.begin();  // SD kart başlatma

  // Mpu 6050 İvme sensörü bağlangıç ayarları.
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);

  // Bmp 280 İçin  Başlangıç ayarları
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Çalışma modu. */
                  Adafruit_BMP280::SAMPLING_X16,    /* Basınç yüksek hızda örnekleme */
                  Adafruit_BMP280::FILTER_X16,      /* Filtreleme. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Bekleme süresi. */
  delay(10);

  eski_irtifa = bmp.readAltitude(1023.7); // Başlangıç irtifasını sıfır yapmak İçin Başlangıç İrtifası kaydetme.
  delay(10);

  int i = 0; // İrtifa dizisine değerlerin atanması
  while (i < 4) {
    irtifa_d[i] = bmp.readAltitude(1023.7);
    i++;
    delay(1);
  }


  int k = 0;  // Açı Dizilerine değerlerin atanması
  while (k < 7) {
    x_dizi[k] = x_aci();
    y_dizi[k] = y_aci();
    k++;
    delay(1);

  }
}
//*****************************************************************************************************************************************************************************
void loop() {

bas:

  // GPS KONUM FALAN xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  durum = 0;
  smartdelay(500);// GPS değerlerinin çekilmesi için gerekli
  
  // GPS 'den enlem boylam değerlerinin çekilmesi
  float enlem , boylam;
  unsigned long age;
  gps.f_get_position(&enlem, &boylam, &age);
  /*Serial.print("Enlem: "); Serial.println(enlem, 8);
  Serial.print("Boylam: "); Serial.println(boylam, 8);*/

  int hiz = gps.f_speed_kmph();// GPS Sensöründen hız değerinin akınması
 // Serial.print("Hız: "); Serial.println(hiz);

  // GPS 'den zaman değerlerinin Çekilmesi
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age2;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age2);
 /* Serial.print("Saat: "); Serial.println(hour + 3);//Utc cinsinden olan saati yerel saate çevirmek için (+3)yaptık
  Serial.print("Dakika: "); Serial.println(minute);
  Serial.print("Saniye: "); Serial.println(second);*/


  // İRTİFA xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  // İrtifa değerlerinin ortalamasının alınması
  irtifa = (irtifa_d[0] + irtifa_d[1] + irtifa_d[2] + irtifa_d[3]) / 4;
  irtifa = irtifa - eski_irtifa;// Bulununan irtifayı fıfır alma işlemi

  irtifa_deger = bmp.readAltitude(1023.7);// Kaydırma İşlemi İçin yeni irtifa değerinin çekilmesi

  /*Serial.print(F("Sıcaklık = "));// Basınç sensöründen sıcaklık alınması
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");

  // İrtifa değerinin ekrana yazdırılması
  Serial.print(F("İrtifa: "));
  Serial.print(irtifa);
  Serial.println(" m");*/*



  //AÇI xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  // Açı değerlerinin ortalamasının alınması
  derece_x = (x_dizi[0] + x_dizi[1] + x_dizi[2] + x_dizi[3] + x_dizi[4] +
              x_dizi[5] + x_dizi[6]) / 7;

  derece_y = (y_dizi[0] + y_dizi[1] + y_dizi[2] + y_dizi[3] + y_dizi[4] +
              y_dizi[5] + y_dizi[6]) / 7;

  //Açı ve ivme değerinin Ekrana Yazdırılması




  if ( irtifa < -200 or (derece_x <= -33.91 and derece_y >= 36.90))   {
    if (m < 10) {
      m++;
      goto bas;

    }
    else if (m >= 10) {

      digitalWrite(9, 1);
      digitalWrite(6, 1);
      delay(6000);
      digitalWrite(6, 0);

      Serial.println("Ana aviyonik kapatıldı.......");
      delay(20000);
      digitalWrite(9, 0);
      while (1) {

        durum = 1;

        data[0] = 0; // X açısı gönderme
        data[1] = 0; // Y açısı gönderme
        data[2] = durum; // X ivmesi gönderme
        data[3] = 0; // hız değeri gönderme
        data[4] = 0; // enlem gönderme
        data[5] = 0; // boylam gönderme
        data[6] = 0;
        verici.write(&data, sizeof(data));
      }
    }
  }
  else {
    m = 0;
    if (derece_x <= 45 and onceki_irtifa  > irtifa + 2 and birinci_ayrilma == 0) {
      birinci_ayrilma++;
      durum = 2;

      digitalWrite(6, 1);

      delay(2000);
      digitalWrite(6, 0);
      data[0] = derece_x; // X açısı gönderme
      data[1] = derece_y; // Y açısı gönderme
      data[2] = durum; // X ivmesi gönderme
      data[3] = hiz; // hız değeri gönderme
      data[4] = enlem, 8; // enlem gönderme
      data[5] = boylam, 8; // boylam gönderme
      data[6] = irtifa;

      verici.write(&data, sizeof(data));



    }

    if ( irtifa < 600 and ikinci_ayrilma == 0 and birinci_ayrilma == 1) {
      ikinci_ayrilma++;
      durum = 3;
      for (int c = 0; c < 8; c++) {
        digitalWrite(6, 1);
        delay(400);
        digitalWrite(6, 0);
        delay(400);
        data[0] = derece_x; // X açısı gönderme
        data[1] = derece_y; // Y açısı gönderme
        data[2] = durum; // X ivmesi gönderme
        data[3] = hiz; // hız değeri gönderme
        data[4] = enlem, 8; // enlem gönderme
        data[5] = boylam, 8; // boylam gönderme
        data[6] = irtifa;

        verici.write(&data, sizeof(data));



      }
    }

  /*  Serial.print("X:"); //X ve Y eksenleri için açı değerleri seri porttan bastım
    Serial.print(derece_x);
    Serial.print("\t");
    Serial.print("Y:");
    Serial.print(derece_y);
    Serial.print("\t");
    Serial.print("X_İvme:");
    Serial.println(x_ivme());*/


    // Dizi kaydırma İşlemi için yeni Açı değerlerinin çekilmesi
    deger_x = x_aci();
    deger_y = y_aci();

    //Açı dizilerinin kaydırma işlemi

    x_dizi[0] = x_dizi[1];
    x_dizi[1] = x_dizi[2];
    x_dizi[2] = x_dizi[3];
    x_dizi[3] = x_dizi[4];
    x_dizi[4] = x_dizi[5];
    x_dizi[5] = x_dizi[6];
    x_dizi[6] = deger_x;

    y_dizi[0] = y_dizi[1];
    y_dizi[1] = y_dizi[2];
    y_dizi[2] = y_dizi[3];
    y_dizi[3] = y_dizi[4];
    y_dizi[4] = y_dizi[5];
    y_dizi[5] = y_dizi[6];
    y_dizi[6] = deger_y;

    // İrtifa dizisinin kaydırma İşlemi
    irtifa_d[0] = irtifa_d[1];
    irtifa_d[1] = irtifa_d[2];
    irtifa_d[2] = irtifa_d[3];
    irtifa_d[3] = irtifa_deger;

    onceki_irtifa = irtifa;

    data[0] = derece_x; // X açısı gönderme
    data[1] = derece_y; // Y açısı gönderme
    data[2] = durum; // X ivmesi gönderme
    data[3] = hiz; // hız değeri gönderme
    data[4] = enlem, 8; // enlem gönderme
    data[5] = boylam, 8; // boylam gönderme
    data[6] = irtifa;

    verici.write(&data, sizeof(data));
    
    Serial.print(hour + 3);//Utc cinsinden olan saati yerel saate çevirmek için (+3)yaptık
    Serial.print("/");
    Serial.print(minute);//DAKİKA
    Serial.print("/");
    Serial.print(second);//SANİYE
    Serial.print("/");
    Serial.print(irtifa);
    Serial.print("/");
    Serial.print(hiz);
    Serial.print("/");
    Serial.print(enlem, 8);
    Serial.print("/");
    Serial.println(boylam, 8);


  }
}








//*************************************************************************************************************************************************************************

// FONKSİYONLAR  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

//X FONKSİYONU**************************************************************

float x_aci() {

  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 6, true);

  int16_t XAxisFull =  (Wire.read() << 8 | Wire.read());
  int16_t YAxisFull =  (Wire.read() << 8 | Wire.read());
  int16_t ZAxisFull =  (Wire.read() << 8 | Wire.read());


  ang_x = (atan(YAxisFull / (sqrt(pow(XAxisFull, 2) + pow(ZAxisFull, 2)))) * 57296 / 1000) + 1.35; //Euler Açı formülüne göre açı hesabı+sensör hatasu (X-Ekseni)

  return ang_x;
}
//****************************************************************************************************************************
//Y FONKSİYONU

float y_aci() {
  Wire.beginTransmission(MPU_addr);                                         //MPU6050 ile I2C haberleşme başlatılır
  Wire.write(0x3B);                                                         //İvme bilgisinin olduğu 0x3B-0x40 için request gönderilir
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 6, true);

  int16_t XAxisFull =  (Wire.read() << 8 | Wire.read());
  int16_t YAxisFull =  (Wire.read() << 8 | Wire.read());
  int16_t ZAxisFull =  (Wire.read() << 8 | Wire.read());

  ang_y = (atan(-1 * XAxisFull / (sqrt(pow(YAxisFull, 2) + pow(ZAxisFull, 2)))) * 57296 / 1000) + 1.64; //Euler Açı formülüne göre açı hesabı+sensör hatası (Y-Ekseni)
  return ang_y;
}


// İVME *******************************************************************************************************************************************************
float x_ivme() {
  Wire.beginTransmission(MPU_addr);                                         //MPU6050 ile I2C haberleşme başlatılır
  Wire.write(0x3B);                                                         //İvme bilgisinin olduğu 0x3B-0x40 için request gönderilir
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 6, true);

  int16_t XAxisFull =  (Wire.read() << 8 | Wire.read());
  int16_t YAxisFull =  (Wire.read() << 8 | Wire.read());
  int16_t ZAxisFull =  (Wire.read() << 8 | Wire.read());
  float XAxisFinal = (float) XAxisFull / 16384.0;                  //Datasheet'te yazan değerlere göre "g" cinsinden ivme buldum. (X ekseni için)
  float YAxisFinal = (float) YAxisFull / 16384.0;
  float ZAxisFinal = (float) ZAxisFull / 16384.0;

  if (XAxisFinal > 0.99) XAxisFinal = 1; //0.99 olan değerler 1'e tamamladım
  if (YAxisFinal > 0.99) YAxisFinal = 1;
  if (ZAxisFinal > 0.99) ZAxisFinal = 1;

  if (XAxisFinal < -0.99) XAxisFinal = -1; //-0.99 olan değerler 1'e tamamladım.
  if (YAxisFinal < -0.99) YAxisFinal = -1;
  if (ZAxisFinal < -0.99) ZAxisFinal = -1;

  return ZAxisFinal;



}


static void smartdelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}
