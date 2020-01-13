#include <ESP8266WiFi.h>
#include <PubSubClient.h>

String ssid;
String pass;

char *ssid0 =  "SmartHouseMoor0.1";  // Имя вайфай точки доступа
char *pass0 =  "30smurchania"; // Пароль от точки доступа

char *ssid1 =  "ELTEX-F892";  // Имя вайфай точки доступа
char *pass1 =  "TG1B001429"; // Пароль от точки доступа

char *ssid2 =  "Halo_of_arduino";  // Имя вайфай точки доступа
char *pass2 =  "radugabezpony"; // Пароль от точки доступа

char *ssid3 =  "SiT_House";  // Имя вайфай точки доступа
char *pass3 =  "sarkimurmur"; // Пароль от точки доступа

char *ssid4 =  "ELTX-2.4GHz_WiFi_5D08";  // Имя вайфай точки доступа
char *pass4 =  "GP2F074740"; // Пароль от точки доступа

char *ssid5 =  "Renat";  // Имя вайфай точки доступа
char *pass5 =  "Mesyacgod"; // Пароль от точки доступа

const char *mqtt_server = "m24.cloudmqtt.com"; // Имя сервера MQTT
const int mqtt_port = 14303; // Порт для подключения к серверу MQTT
const char *mqtt_user = "jjlkmrbf"; // Логи от сервер
const char *mqtt_pass = "0b0beqmnT6Ij"; // Пароль от сервера



WiFiClient wclient;
PubSubClient client(wclient, mqtt_server, mqtt_port);


void setup() {
  
 pinMode (3,INPUT);
 Serial.begin(74880);
 ssid=ssid0;
 pass=pass0;

}

void loop() {
  // подключаемся к wi-fi
  if (WiFi.status() != WL_CONNECTED) {
    //Serial.print("Connecting to ");
    //Serial.print(ssid);
    //Serial.println("...");
    WiFi.begin(ssid, pass);
            if (WiFi.waitForConnectResult()==WL_NO_SSID_AVAIL && ssid==ssid0)
          {ssid=ssid1;pass=pass1;}
       else if (WiFi.waitForConnectResult()==WL_NO_SSID_AVAIL && ssid==ssid1)
          {ssid=ssid2; pass=pass2;}
       else if (WiFi.waitForConnectResult()==WL_NO_SSID_AVAIL && ssid==ssid2)
          {ssid=ssid3; pass=pass3;}
       else if (WiFi.waitForConnectResult()==WL_NO_SSID_AVAIL && ssid==ssid3)
          {ssid=ssid4; pass=pass4;}
       else if (WiFi.waitForConnectResult()==WL_NO_SSID_AVAIL && ssid==ssid4)
          {ssid=ssid5; pass=pass5;};          


    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      {return;};
    //Serial.println("WiFi connected");
  }

  // подключаемся к MQTT серверу
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      //Serial.println("Connecting to MQTT server");
      if (client.connect(MQTT::Connect("arduinoClient2").set_auth(mqtt_user, mqtt_pass))) {
        //Serial.println("Connected to MQTT server");
        client.set_callback(callback);
        client.subscribe("test/lamp1"); // подписывааемся по топик с данными для моей лампы
      } else {
        //Serial.println("Could not connect to MQTT server");   
      }
    }

    if (client.connected()){
      client.loop();
      if (Serial.available()>0)
       {
         RGBWSend();
       };
  } 
  
}
} // конец основного цикла



void RGBWSend()                              // Функция отправки цвета на другую ардуину
{
    String present = Serial.readString();    //записываем инфу от ардуины в строку
    if (present.charAt(3)=='Q' ) //Если третий символ из сериала равен Q
    {           
    client.publish("test/lamp2", present);   //посылаем инфу
    };
}




void callback(const MQTT::Publish& pub)      //функция получения данных с MQTT
{
  String payload = pub.payload_string();
  
  if(String(pub.topic()) == "test/lamp1") // проверяем из нужного ли нам топика пришли данные(lamp1-моя) 
  {    
    String dataSt = String(payload);
    Serial.println(dataSt);
     if (dataSt.charAt(0)=='R'){client.publish("test/lamp2", "Vibro");};
    }
    
  }
