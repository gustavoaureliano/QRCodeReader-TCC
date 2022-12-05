#include <Arduino.h>
#include <ESP32QRCodeReader.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// #include < Wire .h>
#include <LiquidCrystal_I2C.h>

#define col  16 //Define o número de colunas do display utilizado
#define lin   2 //Define o número de linhas do display utilizado
#define ende  0x27 //Define o endereço do display
#define I2C_SDA 14 // SDA Connected to GPIO 14
#define I2C_SCL 15 // SCL Connected to GPIO 15

#define led 33
#define led2 2
#define ledWifi 12
#define buzz 13

const char* ssid = "Kakashi27";
const char* password = "ga87654321";

String serverName = "http://192.168.0.7/web-tcc/api/checkingresso/";

// lcd object 16 columns x 2 rows 
LiquidCrystal_I2C lcd (0x27, 16,2);  //

ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);

void onQrCodeTask(void *pvParameters)
{
  struct QRCodeData qrCodeData;

  while (true)
  {
    if (reader.receiveQrCode(&qrCodeData, 100))
    {
      lcd.clear();
      Serial.println("Found QRCode.");
      lcd. setCursor (0, 0);
      lcd. print ( "Found Qrcode.");
      delay(500);
      if (qrCodeData.valid)
      {
        String idIngresso = (const char *) qrCodeData.payload;
        
        int check = sendRequest(idIngresso);
        
        if (check > 0) {
          Serial.println(idIngresso);
          displayAcessoLiberado();
          digitalWrite(buzz, HIGH);
          delay(1000);
          digitalWrite(buzz, LOW);
          digitalWrite(led2, HIGH);
          delay(4000);
          digitalWrite(led2, LOW);
        } else {
          displayAcessoNegado();
          delay(4000);
        }
        delay(2000);
      }
      else
      {
        String payload = (const char *) qrCodeData.payload;
        
        lcd. setCursor (0, 1);
        lcd. print ( "inv.: ");
        lcd. print ( payload );
        
        digitalWrite(buzz, HIGH);
        delay(200);
        digitalWrite(buzz, LOW);
        
        Serial.print("Invalid: ");
        Serial.println((const char *)qrCodeData.payload);
      }
    }
    displayLerIngresso();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

int sendRequest(String idIngresso) {
  HTTPClient http;
  http.useHTTP10(true); //http 1.0 - httpclient

  String serverPath = serverName + idIngresso;
  //String serverPath = "http://192.168.0.2/web-tcc/api/checkingresso/" + idIngresso;
  Serial.print("URL: ");
  Serial.println(serverPath);
  
  // Your Domain name with URL path or IP address with path
  http.begin(serverPath.c_str());
  
  // Send HTTP GET request
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0 && httpResponseCode < 400) {

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    //Serial.println(payload);
    lcd. setCursor (0, 0);
    lcd. print ( "Resp. code: ");
    lcd. print ( httpResponseCode );
    
    StaticJsonDocument<384> doc;
    DeserializationError error = deserializeJson(doc, http.getStream());

    if (error) {
      lcd. setCursor (0, 0);
      lcd. print ("erro: ");
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      lcd. setCursor (0, 1);
      lcd. print ("erro: ");
      lcd. print (error.c_str());
      return -1;
    }

    int check = doc["check"]; // 1
    return check;
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  return -1;
}

void setupCam() {

  reader.setup();

  Serial.println("Setup QRCode Reader");

  reader.beginOnCore(1);

  Serial.println("Begin on Core 1");

  xTaskCreate(onQrCodeTask, "onQrCode", 4 * 1024, NULL, 4, NULL);
  
}

void setupDisplay() {
  
  Wire.begin(I2C_SDA, I2C_SCL); //  iniciar I2C nas portas especificadas (SDA, SCL)
  lcd.init (); // iniciar o display
  lcd.backlight (); // ligar backlight do display; 
  
}

void setupWifi() {
  
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  lcd.setCursor (0, 0);
  lcd.print ( "Connecting" );
  lcd.setCursor (0, 1);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd. print ( "." );
  }
  Serial.println("");
  digitalWrite(ledWifi, HIGH);
  Serial.print("Connected to WiFi network with IP Address: ");  
  lcd.clear();
  lcd.setCursor (0, 0);
  lcd.print ( "Conn. WiFi, IP:" );
  lcd.setCursor (0, 1);
  lcd.print ( WiFi.localIP() );
  Serial.println(WiFi.localIP());
  delay(1500);
  
}

void displayLerIngresso() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" ETESP Universe ");
  lcd.setCursor(0, 1);
  lcd.print("Ler ingresso....");
}

void displayAcessoLiberado() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" ETESP Universe ");
  lcd.setCursor (0, 1);
  lcd.print ( "Acesso liberado!" );
}

void displayAcessoNegado() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" ETESP Universe ");
  lcd.setCursor (0, 1);
  lcd.print ( "Acesso negado!" );
}

void setup()
{
  setupCam();
  
  pinMode(led, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(ledWifi, OUTPUT);
  pinMode(buzz, OUTPUT);
  
  digitalWrite(led, LOW); //led na placa ligado - indicar que o esp está ligado
  digitalWrite(led2, LOW); //led acesso (verde) desligado
  digitalWrite(ledWifi, LOW); //led wifi (azul) desligado
  
  Serial.begin(115200); //inicar comunicação serial
  Serial.println(); //quebra de linha

  //apitar buzzer indicando o inicio do esp
  digitalWrite(buzz, HIGH);
  delay(500);
  digitalWrite(buzz, LOW);
  delay(500);
  digitalWrite(buzz, HIGH);
  delay(500);
  digitalWrite(buzz, LOW);
  delay(500);
  digitalWrite(buzz, HIGH);
  delay(500);
  digitalWrite(buzz, LOW);
  delay(500);

  setupDisplay();
  setupWifi();
  displayLerIngresso();
  
}

void loop()
{
  delay(100);
}
