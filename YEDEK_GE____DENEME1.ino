#include<SPI.h> //Load SPI Library
#include <Wire.h>  // Wire library - used for I2C communication
#include <Adafruit_BMP085.h>
#include <TinyGPS.h>
TinyGPS gps;
#include <SoftwareSerial.h>
SoftwareSerial ss(3, 4); //rx tx

Adafruit_BMP085 bmp;

double irtifa_deger, irtifa, eski_irtifa; //Bmp 180 basınç sensöründen irtifa değeri okumak için tanımlanan değişkenler.
double irtifa_d[9] = {}; //farklı irtifa değerleri için dizi tanımlama


int ADXL345 = 0x53; // The ADXL345 sensor I2C address

float X_out, Y_out, Z_out, giris_z, eski_z = 0; // Outputs


const byte adres[6] = "00001";

float dizi_z[9] = {};


int eski_zaman = 0;
int led = 7;
int buzzer = 6;

void setup() {
  pinMode(2, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(led, OUTPUT);


  Serial.begin(9600);
  ss.begin(9600);

  SPI.begin();



  Serial.begin(9600);
  Wire.begin(); // Initiate the Wire library
  // Set ADXL345 in measuring mode
  Wire.beginTransmission(ADXL345); // Start communicating with the device
  Wire.write(0x2D); // Access/ talk to POWER_CTL Register - 0x2D
  // Enable measurement
  Wire.write(8); // Bit D3 High for measuring enable (8dec -> 0000 1000 binary)
  Wire.endTransmission();
  delay(5);
  //X-axis
  Wire.beginTransmission(ADXL345);
  Wire.write(0x1E);
  Wire.write(1);
  Wire.endTransmission();
  delay(10);
  //Y-axis
  Wire.beginTransmission(ADXL345);
  Wire.write(0x1F);
  Wire.write(-2);
  Wire.endTransmission();
  delay(10);
  //Z-axis
  Wire.beginTransmission(ADXL345);
  Wire.write(0x20);
  Wire.write(7);
  Wire.endTransmission();
  delay(10);


  Wire.beginTransmission(ADXL345);
  Wire.write(0x32);
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL345, 6, true);

  X_out = ( Wire.read() | Wire.read() << 8);

  Y_out = ( Wire.read() | Wire.read() << 8);

  eski_z = ( Wire.read() | Wire.read() << 8);
  eski_z = eski_z / 256;



  int i = 0;
  while (i < 9) {
    Wire.beginTransmission(ADXL345);
    Wire.write(0x32);
    Wire.endTransmission(false);
    Wire.requestFrom(ADXL345, 6, true);

    X_out = ( Wire.read() | Wire.read() << 8);

    Y_out = ( Wire.read() | Wire.read() << 8);

    dizi_z[i] = ( Wire.read() | Wire.read() << 8);
    dizi_z[i] = dizi_z[i] / 256;

    i++;
  }

  if (!bmp.begin())
  {
    Serial.println("Sensör bulunamadı");
    while (1) {}
  }

  eski_irtifa = bmp.readAltitude(102520); // Başlangıç irtifasını sıfır yapmak İçin Başlangıç İrtifası kaydetme.
  delay(10);

  int  k = 0; // İrtifa dizisine değerlerin atanması
  while (k < 9) {
    irtifa_d[k] = bmp.readAltitude(102520);
    k++;
    delay(1);
  }

}


void loop() {


  if (digitalRead(2) == 1) {
    Serial.print("YEDEK AVİYONİK BAŞLATILDI................");
   

    for (int s = 0; s < 5; s++) {
      digitalWrite(led, 1);
      tone(buzzer, 300);
      delay(100);
      digitalWrite(led, 0 );
      noTone(buzzer);
      delay(100);

    }
    while (1) {

      smartdelay(700);
      Serial.println();

      uint8_t sat = gps.satellites();
      Serial.print("Bağlanılan Uydu Sayısı: "); Serial.println(sat);

      float flat, flon;
      unsigned long age;
      gps.f_get_position(&flat, &flon, &age);
      Serial.print("Enlem: "); Serial.println(flat, 8);
      Serial.print("Boylam: "); Serial.println(flon, 8);

      int alt = gps.f_altitude();
      Serial.print("Rakım: "); Serial.println(alt);

      int spd = gps.f_speed_kmph();
      Serial.print("Hız: "); Serial.println(spd);

      int crs = gps.f_course();
      Serial.print("Rota: "); Serial.println(crs);

      int year;
      byte month, day, hour, minute, second, hundredths;
      unsigned long age2;
      gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age2);

      Serial.print("Saat: "); Serial.println(hour + 3);//Utc cinsinden olan saati yerel saate çevirmek için (+3)yaptık
      Serial.print("Dakika: "); Serial.println(minute);
      Serial.print("Saniye: "); Serial.println(second);


      /* if (millis() - eski_zaman >= 50) {

         eski_zaman = millis();*/

      Wire.beginTransmission(ADXL345);
      Wire.write(0x32);
      Wire.endTransmission(false);
      Wire.requestFrom(ADXL345, 6, true);

      X_out = ( Wire.read() | Wire.read() << 8);
      Y_out = ( Wire.read() | Wire.read() << 8);
      giris_z = ( Wire.read() | Wire.read() << 8);
      giris_z = giris_z / 256;

      Z_out = ((dizi_z[0] + dizi_z[1] + dizi_z[2] + dizi_z[3] + dizi_z[4] + dizi_z[5] + dizi_z[6] +
                dizi_z[7] + dizi_z[8]) / 9);

      Z_out = Z_out - eski_z;

      Serial.print("Z_İVME: ");
      Serial.print(Z_out);
      Serial.print("\t");





      dizi_z[0] = dizi_z[1];
      dizi_z[1] = dizi_z[2];
      dizi_z[2] = dizi_z[3];
      dizi_z[3] = dizi_z[4];
      dizi_z[4] = dizi_z[5];
      dizi_z[5] = dizi_z[6];
      dizi_z[6] = dizi_z[7];
      dizi_z[7] = dizi_z[8];
      dizi_z[8] = dizi_z[9];
      dizi_z[9] = giris_z;


      irtifa = (irtifa_d[0] + irtifa_d[1] + irtifa_d[2] + irtifa_d[3] + irtifa_d[4] + irtifa_d[5] + irtifa_d[6] + irtifa_d[7] + irtifa_d[8]) / 9;
      irtifa = irtifa - eski_irtifa;// Bulununan irtifayı fıfır alma işlemi

      irtifa_deger =  bmp.readAltitude(102520);


      Serial.print("İrtifaa:");
      Serial.println(irtifa);





      // İrtifa dizisinin kaydırma İşlemi
      irtifa_d[0] = irtifa_d[1];
      irtifa_d[1] = irtifa_d[2];
      irtifa_d[2] = irtifa_d[3];
      irtifa_d[3] = irtifa_d[4];
      irtifa_d[4] = irtifa_d[5];
      irtifa_d[5] = irtifa_d[6];
      irtifa_d[6] = irtifa_d[7];
      irtifa_d[7] = irtifa_d[8];
      irtifa_d[8] = irtifa_deger;




      delay(100);
    }
  }


}






static void smartdelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}
