#define FHT_N 256         // массив точек fht
#define LOG_OUT 1         // используем log-output функцию
#include <FHT.h>
#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>
#include <iarduino_OLED_txt.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <SPI.h> 
#include <EEPROM.h>
 
#define PIN 44            // пин ленты DI
#define NUM_LEDS 36       // число диодов ленты
#define PINI 23           // пин индикатора DI
#define NUM_LEDSI 1       // число диодов индикатора
#define ConstFading 10    // скорость затухания и встухания светодиодов. Обратнопропорциональна.
#define ConstFadingPix 5  // скорость затухания светодиодов во время светомузыки
#define CE_PIN 10         //пин для радио се
#define CSN_PIN 53        // пин для радио сsn
#define ColorSmooth 3     //плавность смены цвета светодиодов в режиме цветной 

//Кнопки с первой по восьмую
#define B1 25
#define B2 27
#define B3 29
#define B4 30
#define B5 28
#define B6 26
#define B7 24
#define B8 22

//для повышения дискретизации
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))



RF24 radio(10,53); // объявляем радио модуль
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_RGBW + NEO_KHZ800);
Adafruit_NeoPixel indicator = Adafruit_NeoPixel(NUM_LEDSI, PINI, NEO_RGBW + NEO_KHZ800);
iarduino_OLED_txt myOLED(0x3C);    //объект дисплея
extern uint8_t SmallFontRus[];     //подключаем русский
extern uint8_t SmallFont[];        //подключаем русский

int r,g,b,w=0;             // RGB цвета для индикатора
int r1,g1,b1=0;            //Буфер для выставленного ргб цвета при нажатии кнопки белого
int ShR,ShG,ShB,ShW,ShR1,ShG1,ShB1,ShW1,ShSum=0;     // Shown Red,Green,Blue, White, и такие же для индикатора
boolean mode=0;            //режим настройки цвета RGB/W
byte Option=0;             //флаг меню
String present;            //переменаня для отправки цвета
bool direct=1;             //направление
byte val,valP=0;           //нажатая кнопка, предыдущая нажатая
boolean butt_flag = 0;     //флажок для кнопки управления 6
boolean butt_flag1 = 0;    //флажок для кнопки управления 4
boolean butt_flag2 = 0;    //флажок для кнопки управления Enter
boolean colorORchoise=0;   //переменная отслеживающая выбран в меню мод или любой другой
boolean on_off=1;
boolean ModMenuflag=0;     //флажок для модменю. Если поднят, то считывать энтр для модменю.
boolean modmenuENTER;      //флажок для выбора трека в модменю  
boolean gotKsignal=0;      //флажок получения короткого сигнала
boolean colorChanged=0;    //флажок для проверки соответствия цвета изменяемым промежуткам
boolean trackchosen=0;     //флажок для показа выбранной анимации
boolean buffortrack=0;     //буфер для флажка анимации
boolean on_off_ind=0;      //флажок для включенности индикатора
boolean menushouldchange=1;//флажок для отрисовки меню на экране

boolean LMswitch=0;        //флажок для светомузыки
boolean LmMenuflag=0;      //флажок для меню светомузыки
boolean LMtrackchosen=0;   //флажок для выбора стиля светомузыки в меню светомузыки
byte    LMtrack=0;         //указатель для выбора стиля в меню светомузыки
byte    LMtrackCurrent=0;  //указатель текущего выбранного стиля светомузыки
int     Noise=0;           //переменная для измерения уровня текущего шума
byte    led_counter=0;     //переменная для измерения количества загоревшихся светодиодов
int     PrevSoundLevels[ColorSmooth]; //массив для текущих и предыдущих значений уровня звука
byte    CurrentSoundLevel=0;

byte lin_counter=0;        //счетчик для полос при режиме ограниченной подсветки
unsigned long time;        //переменная для считывания времени нажатия на кнопки.
unsigned long t1,t2,anitime,t3,t4,mainDelay,t5,t6=0;       //переменные для отслеживания времени
int deviceData[4];                         //массив для данных с датчика. 0-номер устройства, 1-запрос, 2-уточнение запроса, 3-данные.

byte track=0;              //отслеживание текущего положения в меню мода 
byte address[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};  //возможные номера труб
byte SNDByte=0;            //переменная для кодов отправки сообщения на устройства. 10 - для чайника.

byte posOffset[12] = {3, 4, 6, 8, 10, 12, 14, 16, 20, 25, 30, 35};  // массив частот для частотного преобразования
byte AllFreqNoise[12][50];     //массив для замера шумов по частотам
byte FreqNoise[12];            //массив с шумами по частотам
byte BeginSoundLevel[15][2];      //массив для порога начала звучания 
byte MinMaxFreqLevel[6][2];      //массив для порога начала звучания 
byte SLcounter=0;              //переменная для счёта порога звучания
int maxsoundlvl=1023;          //переменная для отслеживания максимального уровня текущего звука
int gain;                      //переменная для задания максимума при просчёте звукового уровня

struct pixel      //объект пикселя для запоминания цвета.
{
  byte g;
  byte r;
  byte b;
  byte w;
};

pixel SLstrip[36];   


int LinesBusy[9][9];      //массив для отработки анимации звездопада
byte Firemap[6][6];       //массив для отработки огня


//для огня

#define M_WIDTH 6      // ширина матрицы
#define M_HEIGHT 6      // высота матрицы
#define LED_BRIGHT 255  // яркость

// настройки пламени
#define FIRE_SCALE 30   // масштаб огня
#define HUE_GAP 30      // заброс по hue
#define FIRE_STEP 15    // шаг изменения "языков" пламени
#define HUE_START 10     // начальный цвет огня (0 красный, 80 зелёный, 140 молния, 190 розовый)
#define HUE_COEF 0.6    // коэффициент цвета огня (чем больше - тем дальше заброс по цвету)
#define SMOOTH_K 0.25   // коэффициент плавности огня
#define MIN_BRIGHT 100   // мин. яркость огня
#define MAX_BRIGHT 255  // макс. яркость огня
#define MIN_SAT 200     // мин. насыщенность
#define MAX_SAT 255     // макс. насыщенность

#define ORDER_GRB       // порядок цветов ORDER_GRB / ORDER_RGB / ORDER_BRG
#define COLOR_DEBTH 3   // цветовая глубина: 1, 2, 3 (в байтах)

#define NUM_LEDS M_WIDTH*M_HEIGHT

unsigned char matrixValue[6][6];
unsigned char line[M_WIDTH];
int pcnt = 0;

// ленивая жопа
#define FOR_i(from, to) for(int i = (from); i < (to); i++)
#define FOR_j(from, to) for(int j = (from); j < (to); j++)





void RULE();                        //функция управления RGBW цветом
void GetColor();                    //функция интерпретации воздействия колеса
void SendRGBW();                    //функция отправки цвета на другую лампу
void SetRGBW();                     //функция включения всех светодиодов лампы
void Rejims();                      //функция меню
void ModMenu();                     //функция прорисовки меню в меню спецэффектов
void GetData();                     //функция принятия данных из esp8266
void reMOD();                       //функция управления переключением в меню спецэффектов
void ModMenuEnter();                //функция нажатия энтр в меню спеэффектов
void cast();                        //функция приведения значений показанных и выставленных
void animation();                   //функция проигрывания анимаций
void LinRGBW();                     //функция включения параллельных полос на лампе
void RunTheRainbow();               //функция анимации радуги :3
void Fire();                        //функция имитации огня
void Starfall();                    //функция анимации звездопада
void castIndicator();               //функция плавного выключения индикатора
void gettime();                     //функция запроса и установления времени из модуля esp8266
void NewDeviceMeeting();            //функция знакомства с ардуиной нового устройства 
void Radioturn();                   //функция принятия значений из радиомодуля
void LightMusic();                  //функция проигрывания всех режимов светомузыки
void LightMusicMenu();              //функция меню светомузыки
void LMrechose();                   //функция управления переключением в меню светомузыки
void calibration();                 //функция калибровки микрофона к общему уровню шумов
void Freqcalibration();             //функция калибровки микрофона к уровню шумов по частотам. Происходит в момент калибровки общих шумов
void LMMenuEnter();                 //функция входа в меню
void TurningAllTheStripes(byte quant, byte Q,byte W,byte E,byte R); //функция включения всех полос на ленте в светомузыке
pixel pixelfading(pixel one);       //функция постепенного затухания светодиодов 
pixel colcalibr(bool half,byte i, pixel one); //функция для калибровки значения цвета в зависимости от расположения светодиодов


//перевод Hsv to rgb

typedef struct RgbColor
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} RgbColor;

typedef struct HsvColor
{
    unsigned char h;
    unsigned char s;
    unsigned char v;
} HsvColor;

RgbColor HsvToRgb(HsvColor hsv)
{
    RgbColor rgb;
    unsigned char region, remainder, p, q, t;

    if (hsv.s == 0)
    {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        ShR=rgb.r;
        ShG=rgb.g;
        ShB=rgb.b;
        return rgb;
    }

    region = hsv.h / 43;
    remainder = (hsv.h - (region * 43)) * 6; 

    p = (hsv.v * (255 - hsv.s)) >> 8;
    q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
    t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:
            rgb.r = hsv.v; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = hsv.v; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = hsv.v; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = hsv.v;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = hsv.v;
            break;
        default:
            rgb.r = hsv.v; rgb.g = p; rgb.b = q;
            break;
    }
    ShG=rgb.r;
    ShR=rgb.g/3;
    ShB=rgb.b;
    ShW=0;
    if (ShG/ShR>0.8) {ShW=hsv.v/10;};
    if (ShR<ShG+ShG/8) {ShR=rgb.g/4;};
    if (ShG>200 && ShR<200) {
      map(ShG,0,255,0,170);
      ShW=0;
    };
    String h;
    h="r";
    h+=ShR;
    h+=" g";
    h+=ShG;
    h+=" b";
    h+=ShB;
    return rgb;
}




void setup() {

  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);

  generateLine();                               //огонь
  memset(matrixValue, 0, sizeof(matrixValue));

  radio.begin();               //запускаем радиомодуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0,15);      //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);    //размер пакета, в байтах
  
  radio.openReadingPipe(1,address[0]);      //хотим слушать трубу 0
  radio.openReadingPipe(2,address[1]);      //хотим слушать трубу 0
  radio.openReadingPipe(3,address[2]);      //хотим слушать трубу 0
  radio.openReadingPipe(4,address[3]);      //хотим слушать трубу 0
  radio.openReadingPipe(5,address[4]);      //хотим слушать трубу 0
  radio.openWritingPipe(address[5]);      //хотим слушать трубу 0
  radio.setChannel(0x04);  //выбираем канал (в котором нет шумов!)
  
  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!
  
  // radio.powerUp(); //начать работу
  // radio.startListening();  //начинаем слушать эфир, мы приёмный модуль
  
Serial.begin(74880);
strip.begin();
pinMode(2,OUTPUT);                      // пин вибромотора
pinMode(44,OUTPUT);                     // пин светодиодной ленты 
pinMode(23,OUTPUT);                     // пин индикатора
pinMode(A0,INPUT);                      // пин микрофона
strip.clear();                          // очистить
strip.show();                           // отправить на ленту
indicator.clear();                      // очистить
indicator.show();                       // отправить на ленту
Serial.println("Started");



  pinMode(25,INPUT_PULLUP);  // кнопка 1
  pinMode(27,INPUT_PULLUP);  // кнопка 2
  pinMode(29,INPUT_PULLUP);  // кнопка 3
  pinMode(30,INPUT_PULLUP);  // кнопка 4
  pinMode(28,INPUT_PULLUP);  // кнопка 5
  pinMode(26,INPUT_PULLUP);  // кнопка 6
  pinMode(24,INPUT_PULLUP);  // кнопка 7
  pinMode(22,INPUT_PULLUP);  // кнопка 8
  
    pinMode(A7,INPUT_PULLUP); //пин ардуины 8 - кнопка переключения режимов
    pinMode(A3,INPUT_PULLUP); //пин ардуины 7 - кнопка отправки цвета
    pinMode(A5,INPUT_PULLUP); //пин ардуины 6 - кнопка управления вправо

    myOLED.begin();                  // Инициируем работу с дисплеем
    myOLED.setFont(SmallFontRus);    // Указываем шрифт 
    
    strip.clear();                          // очистить
    strip.show();                           // отправить на ленту


}

void loop() {
time = millis();
GetData();
Rejims();
GetColor();
animation();
Radioturn();
if (LMswitch==true) {LightMusic();};
}





//...............................................................................
//.........................Принятия данных из esp8266............................
//...............................................................................




void GetData()
{
  String Incom="";          //строка принятия из esp8266
  if  (Serial.available()>0){  //Если получаем данные в сериал порт
  Incom=Serial.readString();
   if (Incom.charAt(0)=='r' && Incom.charAt(1)=='g') //Получаем данные из второй лампы
    {
       digitalWrite(2,HIGH);    //Вибрируем дважды в ответ на получение цвета
       delay(300);
       digitalWrite(2,LOW);
       delay(200);
       digitalWrite(2,HIGH);
       delay(300);
       digitalWrite(2,LOW);
       LMswitch=false;    
       if (Incom.charAt(3)=='(')           //
        {
          ShR = Incom.substring(Incom.indexOf('(')+1).toInt();
          ShG = Incom.substring(Incom.indexOf(',')+1,Incom.lastIndexOf(',')).toInt();
          ShB = Incom.substring(Incom.lastIndexOf(',')+1).toInt();
          ShW=0;
        }
       else if (Incom.charAt(3)=='L') 
        {
          ShR = Incom.substring(Incom.indexOf('(')+1).toInt();
          ShG = Incom.substring(Incom.indexOf(',')+1,Incom.lastIndexOf(',')).toInt();
          ShB = Incom.substring(Incom.lastIndexOf(',')+1).toInt();
          ShW = Incom.substring(Incom.indexOf(')')+1).toInt();    
        };
       
       for (byte i=0; i<(NUM_LEDS); i++){
       strip.setPixelColor(i, ShG, ShR, ShB, ShW);}
       strip.show();
       };      
       
       //получаем сигнал о доставке сообщения
       
       if (Incom.charAt(0)=='V')      //Если цвет был доставлен на esp8266-2, то вибрируем 1 раз
         { 
           digitalWrite(2,HIGH);
           delay(300);
           digitalWrite(2,LOW);
         };
       if (Incom.charAt(4)=='K') {gotKsignal=1;buffortrack=trackchosen; trackchosen=0; anitime=time;};
  };
}



//...............................................................................
//.............................Проигрывание анимаций.............................
//...............................................................................


void animation()
{
  //обработка получения короткого сигнала. Если получили К сигнал, то запомнили анимацию, выключили, проиграли К сигнал, и затем вернули анимацию в бывшее значение.
  if (time-anitime>4000 && gotKsignal==1)
{
  gotKsignal=0;
  LMswitch=true;
  cast();
  trackchosen=buffortrack; 
   
};

if (trackchosen==1)
{
switch (track)
{
case 1:  //линии
{
  LinRGBW();
  break;
}
case 2:  //радуга
{
  if (time-t3>30){
  RunTheRainbow();
  t3=time;
  break;
  }
}
case 3:  //Звездопад
{
  Starfall();
  break;
};

case 4:  //Огонь
{
  fireRoutine();
  break;
};

};//конец свича по треку

  
}; //конец выбора режима работы

//выключение индикатора через 10 секунд после окончания его работы
if (time-t4>5000 && on_off_ind==1) {castIndicator();};

}






//...............................................................................
//.....................................МЕНЮ......................................
//...............................................................................



void Rejims()
{
  //Переключение кнопками "влево" "вправо"
  String menu1;    //инфа для меню
  
 if (digitalRead(A5)==1 && butt_flag==0) 
 {
  switch(Option)
  {
  case 0:{if (ModMenuflag==0 && LmMenuflag==0){Option+=1;break;} else if (ModMenuflag==1 && LmMenuflag==0) {trackchosen=0;reMOD();break;} else if (ModMenuflag==0 && LmMenuflag==1){LMtrackchosen=0;LMrechose();break;};}
  case 1:{if (ModMenuflag==0 && LmMenuflag==0){Option+=1;break;} else if (ModMenuflag==1 && LmMenuflag==0) {trackchosen=0;reMOD();break;} else if (ModMenuflag==0 && LmMenuflag==1){LMtrackchosen=0;LMrechose();break;};}
  case 2:{if (ModMenuflag==0 && LmMenuflag==0){Option+=1;break;} else if (ModMenuflag==1 && LmMenuflag==0) {trackchosen=0;reMOD();break;} else if (ModMenuflag==0 && LmMenuflag==1){LMtrackchosen=0;LMrechose();break;};}
  case 3:{if (ModMenuflag==0 && LmMenuflag==0){Option=0; break;} else if (ModMenuflag==1 && LmMenuflag==0) {trackchosen=0;reMOD();break;} else if (ModMenuflag==0 && LmMenuflag==1){LMtrackchosen=0;LMrechose();break;};}
  };
  butt_flag=1;
  menushouldchange=1; //для отрисовки меню
  }
  else if (digitalRead(A5)==0 && butt_flag==1)
  {butt_flag=0;};
  

//Проводим соответствие с дисплеем
if (ModMenuflag==0 && LmMenuflag==0 && menushouldchange==1)
{
  switch (Option) {
  case 0:
  { 
    menu1="\362Отправка цвета\363     "; 
    myOLED.print(menu1,0,0);
    menu1=" Цвет на лампе       ";
    myOLED.print(menu1,0,1);
    menu1=" Светомузыка         ";
    myOLED.print(menu1,0,2);
    menu1=" Специальные эффекты ";
    myOLED.print(menu1,0,3);
    menu1="                     ";
    myOLED.print(menu1,0,4); 
    break;
    }
  case 1:
  { 
    menu1=" Отправка цвета      "; 
    myOLED.print(menu1,0,0);
    menu1="\362Цвет на лампе\363      ";
    myOLED.print(menu1,0,1);
    menu1=" Светомузыка         ";
    myOLED.print(menu1,0,2);
    menu1=" Специальные эффекты ";
    myOLED.print(menu1,0,3);
    menu1="                     ";
    myOLED.print(menu1,0,4);
    break;
    }
  case 2:
   { 
    menu1=" Отправка цвета      "; 
    myOLED.print(menu1,0,0);
    menu1=" Цвет на лампе       ";
    myOLED.print(menu1,0,1);
    menu1="\362Светомузыка\363        ";
    myOLED.print(menu1,1,2);
    menu1=" Специальные эффекты ";
    myOLED.print(menu1,0,3);
    menu1="                     ";
    myOLED.print(menu1,0,4);    
    break;
    }
  case 3:
 { 
    menu1=" Отправка цвета      "; 
    myOLED.print(menu1,0,0);
    menu1=" Цвет на лампе       ";
    myOLED.print(menu1,0,1);
    menu1=" Светомузыка         ";
    myOLED.print(menu1,0,2);
    menu1="\362Специальные эффекты\363";
    myOLED.print(menu1,0,3);
    menu1="                     ";
    myOLED.print(menu1,0,4);    
    break;
    } 
  };
  menushouldchange=0;
};
  
//Реакция на нажатие кнопки Энтр
         if (Option==0 && digitalRead(A3)==1 && butt_flag2==0 && ModMenuflag==0 && LmMenuflag==0) { //выбран пункт *Отпр* и нажата отправка
    t1=time;
    butt_flag2=1;
}    
    else if (Option==1 && digitalRead(A3)==1 && butt_flag2==0 && ModMenuflag==0 && LmMenuflag==0) { //выбран пункт *Ламп* и нажата отправка
    SetRGBW();
    butt_flag2=1;
 }
    else if (Option==2 && digitalRead(A3)==1 && butt_flag2==0 && ModMenuflag==0 && LmMenuflag==0) { //выбран пункт *светомузыка* и нажата отправка
    LightMusicMenu();
    calibration();
    LmMenuflag=1;
    butt_flag2=1;
 }
   else if (Option==2 && digitalRead(A3)==1 && butt_flag2==0 && ModMenuflag==0 && LmMenuflag==1) { //нажатие отправки в лм меню
    LMMenuEnter();
    butt_flag2=1;
 }
    else if (Option==3 && digitalRead(A3)==1 && butt_flag2==0 && ModMenuflag==0 && LmMenuflag==0) { //выбран пункт *специальные эффекты* и нажата отправка
    ModMenu();
    ModMenuflag=1; 
    butt_flag2=1;
 }
    else if (Option==3 && digitalRead(A3)==1 && butt_flag2==0 && ModMenuflag==1 && LmMenuflag==0) { //нажатие отправки в модменю
    ModMenuEnter();
    butt_flag2=1; 
    };
         if (digitalRead(A3)==0 && butt_flag2==1) 
         {
          butt_flag2=0; 
         if (Option==0 && time-t1<2000) { present="rgbQK("; SendRGBW();} 
    else if (Option==0 && time-t1>=2000) { present="rgbQB("; SendRGBW();};
          };  
}


//...............................................................................
//..........................Инициация меню спецэффектов..........................
//...............................................................................



void ModMenu()
{
    String menu1;
    String tim;
    menu1="\362Специальные эффекты\363";
    myOLED.print(menu1,0,0);
    menu1="\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136";
    myOLED.print(menu1,0,1);
    switch (track)
    {
       case 0:
     {
         if (trackchosen==0) {
    tim="\362 Выход             \363";    
    myOLED.print(tim,0,2); 
    tim="\364 Линии              ";
    myOLED.print(tim,0,3); 
    tim="\364 Радуга             ";
    myOLED.print(tim,0,4);
    tim="\364 Звездопад          ";
    myOLED.print(tim,0,5);
    tim="\364 Огонь              ";
    myOLED.print(tim,0,6);
    break;} 
    else if (trackchosen==1) {tim="                     "; ModMenuflag=0;trackchosen=0;menushouldchange=1; for (byte i=4; i<(7); i++){ myOLED.print(tim,0,i);}; break;};

      
     }
     case 1:
     {
    tim="\362 Выход              ";  
    myOLED.print(tim,0,2); 
         if (trackchosen==0) {tim="\364 Линии             \363";} 
    else if (trackchosen==1) {tim="\365 Линии             \363";};
    myOLED.print(tim,0,3); 
    tim="\364 Радуга             ";
    myOLED.print(tim,0,4);
    tim="\364 Звездопад          ";
    myOLED.print(tim,0,5);
    tim="\364 Огонь              ";
    myOLED.print(tim,0,6);
    break;
      
     }
     case 2:
     {
    tim="\362 Выход              ";  
    myOLED.print(tim,0,2); 
    tim="\364 Линии              ";
    myOLED.print(tim,0,3); 
         if (trackchosen==0) {tim="\364 Радуга            \363";} 
    else if (trackchosen==1) {tim="\365 Радуга            \363";castIndicator();};
    myOLED.print(tim,0,4);
    tim="\364 Звездопад          ";
    myOLED.print(tim,0,5);
    tim="\364 Огонь              ";
    myOLED.print(tim,0,6);
    break;
      
     }
     case 3:
     {
    tim="\362 Выход              ";  
    myOLED.print(tim,0,2); 
    tim="\364 Линии              ";
    myOLED.print(tim,0,3); 
    tim="\364 Радуга             ";
    myOLED.print(tim,0,4);
         if (trackchosen==0) {tim="\364 Звездопад         \363";} 
    else if (trackchosen==1) {tim="\365 Звездопад         \363";castIndicator();};
    myOLED.print(tim,0,5);
    tim="\364 Огонь              ";
    myOLED.print(tim,0,6);
    break;
      
     }

     case 4:
     {
    indicator.clear();                          // очистить индикатор
    indicator.show();                           // показать
    tim="\362 Выход              ";  
    myOLED.print(tim,0,2); 
    tim="\364 Линии              ";
    myOLED.print(tim,0,3); 
    tim="\364 Радуга             ";
    myOLED.print(tim,0,4);
    tim="\364 Звездопад          ";
    myOLED.print(tim,0,5);
         if (trackchosen==0) {tim="\364 Огонь             \363";} 
    else if (trackchosen==1) {tim="\365 Огонь             \363";castIndicator();};
    myOLED.print(tim,0,6);
    break;
      
     }
     
      
    };

}



//...............................................................................
//........................Нажатие энтр в меню спеэффектов........................
//...............................................................................


void ModMenuEnter()
{
     if (trackchosen==0){trackchosen=1;} else if (trackchosen==1 && track!=1){trackchosen=0;};
          if (lin_counter<=5 && track==1 && trackchosen==1) {lin_counter+=1;}
     else if (lin_counter=6  && track==1 && trackchosen==1) {lin_counter=0;};
     
   if (track!=0){ 
    strip.clear();                          // очистить
    strip.show();};     
    ModMenu();

}



//...............................................................................
//.............................Режим анимации Радуга ............................
//...............................................................................


void RunTheRainbow()
{
        if (ShR>=0   && ShR<=249  && ShG==0   && ShB==0)   {ShR+=1;} //(250;0;0)
   else if (ShR==250 && ShG>=0    && ShG<=249 && ShB==0)   {ShG+=1;} //(250;250;0)
   else if (ShR>=1  && ShR<=250  && ShG==250 && ShB==0)   {ShR-=1;} //(0;250;0)
   else if (ShR==0   && ShG==250  && ShB>=0   && ShB<=249) {ShB+=1;} //(0;250;250)
   else if (ShR==0   && ShG>=1   && ShG<=250 && ShB==250) {ShG-=1;} //(0;0;250)
   else if (ShR>=0   && ShR<=249  && ShG==0   && ShB==250) {ShR+=1;} //(250;0;250)   
   else if (ShR==250 && ShG==0    && ShB>=1  && ShB<=250) {ShB-=1;}; //(250;0;0)
//   else if (ShR>=1 && ShG<=250 && ShG==0 && ShB==0)   {ShR-=1;}; //(0;0;0)     
   
      for (byte i=0; i<(NUM_LEDS); i++)
     {strip.setPixelColor(i, ShG, ShR, ShB, ShW);}
      strip.show(); 
   
}



//...............................................................................
//.............................Режим анимации звездопада...........................
//...............................................................................


void Starfall()
{
  bool nextstep=true;
  //получаем массив с линиями, по которым будут падать звезды
  if (millis()-t5>700)
  {
  t5=millis();
   for (byte i=0;i<6;i++) 
   {
     LinesBusy[i][5]=random(0,5);
     if (LinesBusy[i][4]==1) {strip.setPixelColor(i*6+5, g, r, b, w+30);};
   };
   strip.show();
  };
  //задаём алгоритм падения звёзд
  if (millis()-t6>75)
  {

    t6=millis();
    
  for (int i=0;i<6;i++) 
   {
    for (int j=5;j>=0;j--) 
    {
       if (nextstep==false) {nextstep=true; continue;};
       if  (LinesBusy[i][j]==1 && j!=0)
       { 
         if (j-1>=0)  {LinesBusy[i][j-1]=1; strip.setPixelColor(i*6+ j-1, g, r, b, w+30);};
         if (j>=0)    {LinesBusy[i][j]=3; strip.setPixelColor(i*6+j, g/3, r/3, b/3, w+10);};
         if (j+1<=5)  {LinesBusy[i][j+1]=9; strip.setPixelColor(i*6+ j+1, g/9, r/9, b/9, w);};
         if (j+2<=5) { strip.setPixelColor(i*6+j+2, 0,0,0,0);};
         if (j+3<=5) { strip.setPixelColor(i*6+j+3, 0,0,0,0);};
        nextstep=false;
        //continue;
      }
      else if  (LinesBusy[i][j]==1 && j==0) 
      {
         if (j>=0)   {LinesBusy[i][j]=3; strip.setPixelColor(i*6+j, g/3, r/3, b/3, w+10);};
         if (j+1<=5) {LinesBusy[i][j+1]=9; strip.setPixelColor(i*6+j+1, g/9, r/9, b/9, w);};
         if (j+2<=5) { strip.setPixelColor(i*6+j+2, 0,0,0,0);};
         if (j+3<=5) { strip.setPixelColor(i*6+j+3, 0,0,0,0);};
        nextstep=false;
        //continue;
      }
      else if ((j==0) && (LinesBusy[i][j]==3))
      {
        LinesBusy[i][j]=9;  
        strip.setPixelColor(i*6+j, g/9, r/9, b/9, w);
        strip.setPixelColor(i*6+j+1, 0,0,0,0);
        strip.setPixelColor(i*6+j+2, 0,0,0,0);
        nextstep=false;
        //continue;
      }
      else if ((j==0) && (LinesBusy[i][j]==9))
      {
        LinesBusy[i][j]=0;  
        strip.setPixelColor(i*6+j, 0,0,0,0);
        strip.setPixelColor(i*6+j+1, 0,0,0,0);
        strip.setPixelColor(i*6+j+2, 0,0,0,0);
        nextstep=false;
        //continue;
      }
      else {};
           
      };
    };
   strip.show(); 
  };

}
//...............................................................................
//..............................Отправка RGBW цвета..............................
//...............................................................................



void SendRGBW()
{
if (time-t2>2000)        //кулдаун отправки
{
present+=r;
present+=',';
present+=g;
present+=',';
present+=b;
present+=')';
present+=w;
Serial.print(present);
t2=time;
};  
 
}




//...............................................................................
//.......................Включение всех светодиодов лампы........................
//...............................................................................



void SetRGBW()
{
  if (ShG!=g || ShR!=r || ShB!=b || ShW!=w) 
    {
    on_off=1;
    cast();
    }
    
else if (ShG==g && ShR==r && ShB==b && ShW==w && on_off==1)
{
   indicator.clear();                          // очистить индикатор
   indicator.show();                           // показать
   on_off=0;
   cast();   
  } 
  
  else if (on_off==0) 
  {        
    indicator.setPixelColor(0,g,r,b,w);    // восстановить индикатор
    indicator.show();                               // восстановить ленту  
    on_off=1;
    cast();
    };
   
}




//...............................................................................
//.......................Интерпретации воздействия колеса........................
//...............................................................................


void GetColor()
{
bool IndShouldChange=0;         //флажок для изменения состояния индикатора и связанного с ним вывода цвета
mode=digitalRead(A7);           //Если кнопка нажата, то редактируем White
                                //Если кнопка не нажата, то редактируем RGB
    if (digitalRead(B1)==1)
      {
        valP=val; //предыдущее значение вал
        val=B1;    //текущее значение вал
        switch(valP)
        {
          case B1:{break;}
          case B7:{direct=1; RULE();break;}
          case B8:{direct=1; RULE();break;}
          default:{direct=0; RULE();break;}          
        };
        IndShouldChange=1;
       };

           if (digitalRead(B2)==1)
      {
        valP=val; //предыдущее значение вал
        val=B2;    //текущее значение вал
        switch(valP)
        {
          case B1:{direct=1; RULE();break;}
          case B2:{break;}
          case B8:{direct=1; RULE();break;}
          default:{direct=0; RULE();break;}      
        };
        IndShouldChange=1;
       };

        if (digitalRead(B3)==1)
      {
        valP=val; //предыдущее значение вал
        val=B3;    //текущее значение вал
        switch(valP)
        {
          case B1:{direct=1; RULE();break;}
          case B2:{direct=1; RULE();break;}
          case B3:{break;}
          default:{direct=0; RULE();break;}      
        };
        IndShouldChange=1;
       };

       if (digitalRead(B4)==1)
      {
        valP=val; //предыдущее значение вал
        val=B4;    //текущее значение вал
        switch(valP)
        {
          case B2:{direct=1; RULE();break;}
          case B3:{direct=1; RULE();break;}
          case B4:{break;}
          default:{direct=0; RULE();break;}          
        };
        IndShouldChange=1;
       };

       if (digitalRead(B5)==1)
      {
        valP=val; //предыдущее значение вал
        val=B5;    //текущее значение вал
        switch(valP)
        {
          case B3:{direct=1; RULE();break;}
          case B4:{direct=1; RULE();break;}
          case B5:{break;}
          default:{direct=0; RULE();break;}        
        };
        IndShouldChange=1;
       };

       if (digitalRead(B6)==1)
      {
        valP=val; //предыдущее значение вал
        val=B6;    //текущее значение вал
        switch(valP)
        {

          case B4:{direct=1; RULE();break;}
          case B5:{direct=1; RULE();break;}
          case B6:{break;}
          default:{direct=0; RULE();break;}       
        };
        IndShouldChange=1;
       };

       if (digitalRead(B7)==1)
      {
        valP=val; //предыдущее значение вал
        val=B7;    //текущее значение вал
        switch(valP)
        {
          case B5:{direct=1; RULE();break;}
          case B6:{direct=1; RULE();break;}
          case B7:{break;}
          default:{direct=0; RULE();break;}       
        };
        IndShouldChange=1;
       };

       if (digitalRead(B8)==1)
      {
        valP=val; //предыдущее значение вал
        val=B8;    //текущее значение вал
        switch(valP)
        {
          case B6:{direct=1; RULE();break;}
          case B7:{direct=1; RULE();break;}
          case B8:{break;}  
          default:{direct=0; RULE();break;}     
        };
        IndShouldChange=1;
       };

        if (IndShouldChange==1)
        {
        indicator.clear(); 
        t4=time; 
        on_off_ind=1; 
        indicator.setPixelColor(0, g, r, b, w);
        indicator.show();
            //Прости меня, мой я. Это для индикатора, чтобы он норм работал.
            //    ShR1=r;  ShB1=b;   ShG1=g;  ShW1=w;
    
        String present1;
        
present1+=" RGBW(";
present1+=r;
present1+=',';
present1+=g;
present1+=',';
present1+=b;
present1+=',';
present1+=w;
present1+=")       ";
 //        myOLED.clrScr();
         myOLED.print(present1,0,7);  
        IndShouldChange=0;
        };
}


//...............................................................................
//............................Управление RGBW цветом.............................
//...............................................................................




void RULE() 
{
switch(mode)               //Изменяем RGB и White
{
case 0:{                   //Изменяем RGB компонент
  if (colorChanged==1)     //Если цвет был изменен и, может быть, выпал из промежутков, то вернем ему его цвет
  {
r=r1;
g=g1;
b=b1;
colorChanged=0;
      };     

  
      switch(direct) {  //Если положение:

           case 0: {         //Отрицательное
        
        if (r==0 && g==0 && b>=0 && b<=240)    {b+=10;} //(0;0;250)
   else if (r==0 && g>=0 && g<=240 && b==250)  {g+=10;} //(0;250;250)
   else if (r==0 && g==250 && b>=10 && b<=250) {b-=10;} //(0;250;0)
   else if (r>=0 && r<=240 && g==250 && b==0)  {r+=10;} //(250;250;0)
   else if (r==250 && g<=250 && g>=10 && b==0) {g-=10;} //(250;0;0)
   else if (r==250 && g==0 && b>=0 && b<=240)  {b+=10;} //(250;0;250)   
   else if (r<=250 && r>=10 && g==0 && b==250) {r-=10;} //(0;0;250)
   else if (r>=10 && r<=250 && g==0 && b==0)   {r-=10;}; //(0;0;0)    
       
        break;}

      
      case 1:{          //Положительное 
        
        if (r>=0 && r<=240 && g==0 && b==0)    {r+=10;} //(250;0;0)
   else if (r==250 && g>=0 && g<=240 && b==0)  {g+=10;} //(250;250;0)
   else if (r>=10 && r<=250 && g==250 && b==0) {r-=10;} //(0;250;0)
   else if (r==0 && g==250 && b>=0 && b<=240)  {b+=10;} //(0;250;250)
   else if (r==0 && g>=10 && g<=250 && b==250) {g-=10;} //(0;0;250)
   else if (r>=0 && r<=240 && g==0 && b==250)  {r+=10;} //(250;0;250)   
   else if (r==250 && g==0 && b>=10 && b<=250) {b-=10;} //(250;0;0)  
   else if (r==0 && g==0 && b>=10 && b<=250)   {b-=10;}; //(0;0;0) 
      break;}
      
      }        //Конец зависимости от положения
  
      break;}  //Конец изменения RGB компонента
      
case 1:{ //Изменяем White компонент
  if (colorChanged==0) // если мы увеличивали вайт до запредельных значений
  {
  colorChanged=1;
  r1=r;
  g1=g;
  b1=b;
  };
  switch(direct)
  {
    case 0: {if (w>=5 && r1==r && g1==g && b1==b)   {w-=5;} else if (w==255) {if (r>r1)  {r-=5;}; if (g>g1){g-=5;}; if (b>b1){b-=5;};}; break;}
    
    case 1: {if (w<=250 && r1==r && g1==g && b1==b) {w+=5;} else if (w==255) {if (r<=250){r+=5;}; if (g<=250){g+=5;}; if (b<=250){b+=5;};};break;}
  }; //Конец изменения оттенка white
};   //Конец изменения White компонента 
};   //Конец изменения RGB и White

} // Конец функции




//...............................................................................
//....................Включение параллельных полос на лампе......................
//...............................................................................



void LinRGBW()
{
 
 byte NUM_LEDS_LIN;
 NUM_LEDS_LIN=lin_counter*6;  //в финальной версии добавить "*6" 
     for (byte i=0; i<(NUM_LEDS_LIN); i++)
     {strip.setPixelColor(i, g, r, b, w);}
     
     strip.show(); 
  }


//...............................................................................
//.........................Управление в меню спецэффектов........................
//...............................................................................



void reMOD()
{  
    switch(track) {  
         case 0: {track+=1; break;}
         case 1: {track+=1; break;}
         case 2: {track+=1; break;}
         case 3: {track+=1; break;}
         case 4: {track=0;  break;}
  };
  ModMenu();
}




//...............................................................................
//..................приведение значений показанных и выставленных................
//...............................................................................



void cast()
{
    butt_flag2=1;
    
    //Коэффициенты затухания/востухания
    float RR,GG,BB,WW,rr,gg,bb,ww=0;
    RR=ShR/ConstFading; rr=r/ConstFading;
    GG=ShG/ConstFading; gg=g/ConstFading;
    BB=ShB/ConstFading; bb=b/ConstFading;
    WW=ShW/ConstFading; ww=w/ConstFading;
   
    
  if (on_off==1){                                  //приведение цвета
    
    byte RRr,GGg,BBb,WWw=0;                        //коэффициенты для плавного приведения цвета
      if (ShR>r) {RRr=floor((r-ShR)/ConstFading);} else if (ShR<r) {RRr=ceil((r-ShR)/ConstFading);} else if (ShR==r){RRr=0;};
      if (ShG>g) {GGg=floor((g-ShG)/ConstFading);} else if (ShG<g) {GGg=ceil((g-ShG)/ConstFading);} else if (ShG==g){GGg=0;};
      if (ShB>b) {BBb=floor((b-ShB)/ConstFading);} else if (ShB<b) {BBb=ceil((b-ShB)/ConstFading);} else if (ShB==b){BBb=0;};
      if (ShW>w) {WWw=floor((w-ShW)/ConstFading);} else if (ShW<w) {WWw=ceil((w-ShW)/ConstFading);} else if (ShW==w){WWw=0;};


      for (byte k=0; k<ConstFading; k++) 
    {
     ShR+=RRr;
     ShG+=GGg;
     ShB+=BBb;
     ShW+=WWw;
     
          for (byte pq=0; pq<(NUM_LEDS); pq++){
               strip.setPixelColor(pq, ShG, ShR, ShB, ShW);
               strip.show(); 
               };
               //delay(10);
               };
               
               if (ShR!=r) {ShR=r;};
               if (ShG!=g) {ShG=g;};
               if (ShB!=b) {ShB=b;};
               if (ShW!=w) {ShW=w;};
               
               for (byte pq1=0; pq1<(NUM_LEDS); pq1++){
               strip.setPixelColor(pq1, ShG, ShR, ShB, ShW);
               strip.show(); 
               };
    }
   else if (on_off==0)
   {
    for (byte j=0; j<ConstFading; j++)
    {
     if (ShR-RR>RR){ShR-=ceil(RR);} else if (ShR-RR<=RR){ShR=0;};
     if (ShG-GG>GG){ShG-=ceil(GG);} else if (ShG-GG<=GG){ShG=0;};
     if (ShB-BB>BB){ShB-=ceil(BB);} else if (ShB-BB<=BB){ShB=0;};
     if (ShW-WW>WW){ShW-=ceil(WW);} else if (ShW-WW<=WW){ShW=0;};
     
               //заполняем ленту цветами
               for (byte i=0; i<(NUM_LEDS); i++){
               strip.setPixelColor(i, ShG, ShR, ShB, ShW);
               strip.show(); 
               };
               //delay(10);
    };
    
   }
       butt_flag2=0;
  }


//...............................................................................
//..................плавное выключение индикатора................................
//...............................................................................



  void castIndicator()
  {
    on_off_ind=0;
    
    
    float RR,GG,BB,WW=0;
    RR=ShR1/ConstFading; 
    GG=ShG1/ConstFading; 
    BB=ShB1/ConstFading; 
    WW=ShW1/ConstFading; 
   

   for (byte j=0; j<ConstFading; j++)
    {
     if (ShR1-RR>RR){ShR1-=ceil(RR);} else if (ShR1-RR<=RR){ShR1=0;};
     if (ShG1-GG>GG){ShG1-=ceil(GG);} else if (ShG1-GG<=GG){ShG1=0;};
     if (ShB1-BB>BB){ShB1-=ceil(BB);} else if (ShB1-BB<=BB){ShB1=0;};
     if (ShW1-WW>WW){ShW1-=ceil(WW);} else if (ShW1-WW<=WW){ShW1=0;};
     
               indicator.setPixelColor(0, ShG1, ShR1, ShB1, ShW1);
               indicator.show(); 
               delay(10);
    };
    
  }

  
//...............................................................................
//....................Знакомство нового устройства с ардуиной....................
//...............................................................................


//void NewDeviceMeeting()
//{
//  
//}




//...............................................................................
//..........................принятие значений из радиомодуля.....................
//...............................................................................

void Radioturn()
{
  /*
    byte pipeNo;
    String sendingDevData;
    
while ( radio.available(&pipeNo) ) {                      //cлушаем эфир со всех труб
    radio.read( &deviceData, sizeof(deviceData));};         // чиатем входящий сигнал

    if (deviceData[0]==255) {NewDeviceMeeting();}
    else if (deviceData[0]<250) {
      switch (deviceData[1])
      {
        case 1:{sendingDevData Serial.print("Send");break;}
        
      };
    }

//if (gotByte==111) 
//{gotByte=0; 
//Serial.println("casting..."); 
//if (trackchosen==1) {trackchosen=0;} 
//else if (trackchosen==0){trackchosen=1;};
//SetRGBW();};

*/



}

//...............................................................................
//..........................отправление инфы с радиомодуля.......................
//...............................................................................

void Radiosend()
{
 if (SNDByte!=0) 
 {
 radio.stopListening();                     //заканчиваем слушать эфир
 radio.write (&SNDByte, sizeof(SNDByte));   //отправляем по шестому радиоканалу
 SNDByte=0;                                 //обратно зануляем снд байт
 radio.startListening();                    //заканчиваем слушать эфир
 };
}



//...............................................................................
//..............функция одновременного включения полос...........................
//...............................................................................
void TurningAllTheStripes (byte quant, byte Q, byte W, byte E, byte R)
{
for (byte i=0; i<quant; i++) 
{
strip.setPixelColor(i,    Q, W, E, R);
strip.setPixelColor(i+6,  Q, W, E, R);
strip.setPixelColor(i+12, Q, W, E, R);
strip.setPixelColor(i+18, Q, W, E, R);
strip.setPixelColor(i+24, Q, W, E, R);
strip.setPixelColor(i+30, Q, W, E, R); 
};
}


  


//...............................................................................
//.....................проигрывание всех режимов светомузыки.....................
//...............................................................................


void LightMusic()
{
int RcurrentLevel, RsoundLevel, bufcolor,LMR,LMG,LMB,LMW,BufColor=0;

        int sample=0;
        RsoundLevel=0;
        if (millis() - mainDelay > 5) {       // итерация отработки анимации
         mainDelay = millis();
         for (int i = 0 ; i < FHT_N ; i++)    // функция FHT, забивает массив fht_log_out[] величинами по спектру
         {
         sample = analogRead(A1);
         fht_input[i] = sample; // put real data into bins
         if (sample>RsoundLevel) {RsoundLevel=sample;};
         };
         fht_window();  // window the data for better frequency response
         fht_reorder(); // reorder the data before doing the fht
         fht_run();     // process the data in the fht
         fht_mag_log(); // take the output of the fht
         RsoundLevel*=1.2;
         RsoundLevel = map(RsoundLevel, Noise, 1023, 0, 450);
         RsoundLevel = constrain(RsoundLevel, 0, 200);         
        };
   switch(LMtrackCurrent)
     {
       case 0: {break;}
               
       case 1: { 
                  int allfreq[12];              //массив для отображаемых частот
                  //выделяем из полученного спектра определенные частоты
                  for (byte freq = 0; freq < 12; freq++) 
                      {    
                        allfreq[freq] = fht_log_out[posOffset[freq]];
                      };
                  //Фильтруем полученные частоты по уровню шумов
                  for (byte freq = 0; freq < 12; freq++) 
                      {    
                         if (allfreq[freq]<=FreqNoise[freq]) allfreq[freq]=0;
                      };
                      //раз в 10 секунд обновляем максимальное и минимальное значение, и заносим его в массив 6.
                      //каждую итерацию мапим от минимального за 10 секунд значения до максимального каждую из 6ти строк светодиодов.
                      
                      for (byte i = 0; i < 6; i++) 
                      {
                        if (MinMaxFreqLevel[i][0]==0) {MinMaxFreqLevel[i][0]=50;};
                        if(  ((allfreq[i]+allfreq[i+6])/2)<MinMaxFreqLevel[i][0]&&(allfreq[i]+allfreq[i+6])/2!=0) 
                             {
                               MinMaxFreqLevel[i][0]=(allfreq[i]+allfreq[i+6])/2;
                             };
                        if(  ((allfreq[i]+allfreq[i+6])/2)>MinMaxFreqLevel[i][1]) 
                             {
                               MinMaxFreqLevel[i][1]=(allfreq[i]+allfreq[i+6])/2;
                             };
                      };
                      
              for (int j=0; j<6;j++)
              {
                for (int i=0; i<6;i++)
               {
                  SLstrip[i+6*j]=pixelfading(SLstrip[i+6*j]);  
                  strip.setPixelColor(i+6*j,SLstrip[i+6*j].g, SLstrip[i+6*j].r, SLstrip[i+6*j].b, SLstrip[i+6*j].w);      
               };
              };


              
                   byte partcolor=0;  
                   for (byte i = 0; i < 6; i++) 
                      {
                        partcolor=(allfreq[i*2]+allfreq[i*2+1])/2;
                        partcolor=constrain(partcolor,MinMaxFreqLevel[i][0], MinMaxFreqLevel[i][1]);
                        partcolor=map(partcolor, MinMaxFreqLevel[i][0], MinMaxFreqLevel[i][1], 0, 8);
                        if (partcolor>1 && partcolor<7)
                         {  
                          for (byte j=0;j<partcolor/2;j++)
                             {
                              if (j==partcolor/2-1 && partcolor%2==1)
                                {
                                  SLstrip[18+i+6*j]=colcalibr(true,i,SLstrip[18+i+6*j]);
                                  strip.setPixelColor(18+i+6*j,SLstrip[18+i+6*j].g, SLstrip[18+i+6*j].r, SLstrip[18+i+6*j].b, SLstrip[18+i+6*j].w);
                                }
                                else 
                                 {
                                  SLstrip[18+i+6*j]=colcalibr(false,i,SLstrip[18+i+6*j]);
                                  strip.setPixelColor(18+i+6*j,SLstrip[18+i+6*j].g, SLstrip[18+i+6*j].r, SLstrip[18+i+6*j].b, SLstrip[18+i+6*j].w);  
                                  };
                             };
                          for (byte j=0;j<partcolor/2;j++)
                              {
                                if (j==partcolor/2-1 && partcolor%2==1)
                                {
                                  SLstrip[18+i-6*j]=colcalibr(true,i,SLstrip[18+i-6*j]);
                                  strip.setPixelColor(18+i-6*j,SLstrip[18+i-6*j].g, SLstrip[18+i-6*j].r, SLstrip[18+i-6*j].b, SLstrip[18+i-6*j].w); 
                                }
                                else {
                                  SLstrip[18+i-6*j]=colcalibr(false,i,SLstrip[18+i]);
                                  strip.setPixelColor(18+i-6*j,SLstrip[18+i-6*j].g, SLstrip[18+i-6*j].r, SLstrip[18+i-6*j].b, SLstrip[18+i-6*j].w);  
                                };
                              };
                         } else if (partcolor==1) 
                           {
                            SLstrip[18+i]=colcalibr(true,i,SLstrip[18+i]);
                            strip.setPixelColor(18+i,SLstrip[18+i].g, SLstrip[18+i].r, SLstrip[18+i].b, SLstrip[18+i].w); 
                           }
                           else {};
                      };
                      strip.show();
        break;}
               
       case 2: {
         int Soundlevelplus=0;
              if (RsoundLevel<=30) {Soundlevelplus=0;}
         else if (RsoundLevel>30  && RsoundLevel<=60)  {Soundlevelplus=1;}
         else if (RsoundLevel>60  && RsoundLevel<=90)  {Soundlevelplus=2;}
         else if (RsoundLevel>90  && RsoundLevel<=120) {Soundlevelplus=3;} 
         else if (RsoundLevel>120 && RsoundLevel<=150) {Soundlevelplus=4;} 
         else if (RsoundLevel>150 && RsoundLevel<=180) {Soundlevelplus=5;} 
         else if (RsoundLevel>180)  {Soundlevelplus=6;}; 
         
         int maxC=0;   //максимальный уровень частоты до 4кГц, отображаемый цветом  
         int maxCi=0;  //порядковый номер максимальной частоты для первой части спектра

         int allfreq[12];              //массив для отображаемых частот
         //выделяем из полученного спектра определенные частоты
         for (byte freq = 0; freq < 12; freq++) 
           {    
              allfreq[freq] = fht_log_out[posOffset[freq]];
            };
         //Фильтруем полученные частоты по уровню шумов
         for (byte freq = 0; freq < 12; freq++) 
           {    
              if (allfreq[freq]<=FreqNoise[freq]) allfreq[freq]=0;
              if (allfreq[freq]>maxC)  {maxC=allfreq[freq]; maxCi=freq;};
            };
         maxCi=(maxCi+1)/2;

         //блок фильтрации неисключаемых случайных шумов
         int SLmed=0;             //медиана значения частот за последние 15 итераций
         int SLmedamp=0;          //медиана значения амплитуды за последние 15 итераций
         BeginSoundLevel[SLcounter][0]=maxCi;
         BeginSoundLevel[SLcounter][1]=Soundlevelplus;
         SLcounter++;
         if (SLcounter>=15) SLcounter=0;
         for (byte i=0;i<15;i++)
           {
              SLmed+=BeginSoundLevel[i][0];
              SLmedamp+=BeginSoundLevel[i][1];
           };
         SLmed/=15;
         SLmedamp/=15;
         if (SLmed<1) maxCi=0;
         if (SLmedamp<1) Soundlevelplus=0;
         int finallevel=0;                 //переменная для финального просчёта количества загоревшихся светодиодов
         finallevel=(maxCi+Soundlevelplus)/2;
         for (int j=0; j<6;j++)
            {
           for (int i=0; i<6;i++)
             {
               if (finallevel>i)
                {
                  SLstrip[i+6*j].g=g;  
                  SLstrip[i+6*j].r=r;  
                  SLstrip[i+6*j].b=b;  
                  SLstrip[i+6*j].w=w;               
                }
                else 
                {
                  SLstrip[i+6*j]=pixelfading(SLstrip[i+6*j]);       
                };
                  strip.setPixelColor(i+6*j,SLstrip[i+6*j].g, SLstrip[i+6*j].r, SLstrip[i+6*j].b, SLstrip[i+6*j].w); 
              };
            };
          strip.show();             
          break;}


          
     case 3: {
                  byte quantity=0;            //количество загоревшихся светодиодов в зависимости от уровня звука
                  if      (RsoundLevel>30 && RsoundLevel<=60) {quantity=0;}
                  else if (RsoundLevel>60 && RsoundLevel<=90) {quantity=5;}
                  else if (RsoundLevel>90 && RsoundLevel<=120) {quantity=10;}
                  else if (RsoundLevel>120 && RsoundLevel<=150) {quantity=15;}
                  else if (RsoundLevel>150 && RsoundLevel<=180) {quantity=20;}
                  else if (RsoundLevel>180) {quantity=25;}
                  else {quantity=0;};
                  
                    byte usednum[30];           //массив с номерами светодиодов, которые загорятся
                    for (byte i=0;i<quantity;i++)
                    {
                     byte randnum=random(36);  //случайный номер загорающегося светодиода
                     bool isunique=true;       //флажок уникальности порядкового номера светодиода
                     do
                     {
                      isunique=true;
                      for (byte j=0; j<quantity;j++)
                      {
                       if (usednum[j]==randnum) 
                        {
                          randnum=random(36); 
                          isunique=false;
                        };
                      };                 
                     } while (isunique==false); 
                     
                     usednum[i]=randnum;
                    };
                    for (byte i=0;i<36;i++)
                    {
                      SLstrip[i]=pixelfading(SLstrip[i]);
                      strip.setPixelColor(i, SLstrip[usednum[i]].g, SLstrip[usednum[i]].r, SLstrip[usednum[i]].b, 0);   
                    };

                    if (t5<millis())
                    {
                      t5=millis()+150;
                    for (byte i=0;i<quantity;i++)
                    {
                      SLstrip[usednum[i]].g=random(10,250);  
                      SLstrip[usednum[i]].r=random(10,250);  
                      SLstrip[usednum[i]].b=random(10,250);  
                      strip.setPixelColor(usednum[i], SLstrip[usednum[i]].g, SLstrip[usednum[i]].r, SLstrip[usednum[i]].b, 0);
                    };
                    };
                    strip.show();
                  break;}      
                };
                             

            /*{for (byte i=0; i<(NUM_LEDS); i++) 
                      {strip.setPixelColor(i, 0, 0, 0, 0);};  
                      strip.show();              
              };*/
} 



//...............................................................................
//...............................меню режимов светомузыки........................
//...............................................................................


void LightMusicMenu()
{
    String menu1;
    String tim;
    menu1="\362Меню светомузыки   \363";  
    myOLED.print(menu1,0,0);
    menu1="\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136\136";
    myOLED.print(menu1,0,1);
    switch (LMtrack)
    {
       case 0:
     {
         if (LMtrackchosen==0) {
    tim="\362 Выход             \363";    
    myOLED.print(tim,0,2); 
    tim="\364 Цветочастотный     ";
    myOLED.print(tim,0,3); 
    tim="\364 Уровневый          ";
    myOLED.print(tim,0,4);
    tim="\364 Небагафичевый      ";
    myOLED.print(tim,0,5);
    break;} 
    else if (LMtrackchosen==1) {tim="                     "; LmMenuflag=0; LMtrackchosen=0; menushouldchange=1; LMswitch=false; for (byte i=4; i<(7); i++){ myOLED.print(tim,0,i);}; break;};

      
     }
     case 1:
     {
    tim="\362 Выход              ";  
    myOLED.print(tim,0,2); 
         if (LMtrackchosen==0) {tim="\364 Цветочастотный    \363";} 
    else if (LMtrackchosen==1) {tim="\365 Цветочастотный    \363";castIndicator();};
    myOLED.print(tim,0,3); 
    tim="\364 Уровневый          ";
    myOLED.print(tim,0,4);
    tim="\364 Небагафичевый      ";
    myOLED.print(tim,0,5);
    break;
      
     }
     case 2:
     {
    tim="\362 Выход              ";  
    myOLED.print(tim,0,2); 
    tim="\364 Цветочастотный     ";
    myOLED.print(tim,0,3); 
         if (LMtrackchosen==0) {tim="\364 Уровневый         \363";} 
    else if (LMtrackchosen==1) {tim="\365 Уровневый         \363";castIndicator();};
    myOLED.print(tim,0,4);
    tim="\364 Небагафичевый      ";
    myOLED.print(tim,0,5);
    break;
      
     }
     case 3:
     {
    indicator.clear();                          // очистить индикатор
    indicator.show();                           // показать
    tim="\362 Выход              ";  
    myOLED.print(tim,0,2); 
    tim="\364 Цветочастотный     ";
    myOLED.print(tim,0,3); 
    tim="\364 Уровневый          ";
    myOLED.print(tim,0,4);
         if (LMtrackchosen==0) {tim="\364 Небагафичевый     \363";} 
    else if (LMtrackchosen==1) {tim="\365 Небагафичевый     \363";castIndicator();};
    myOLED.print(tim,0,5);
    break;
      
     }
     
      
    };

  
}


//...............................................................................
//....................управление положением в меню светомузыки...................
//...............................................................................


void LMrechose()
{
  switch(LMtrack) {  
         case 0: {LMtrack+=1; break;}
         case 1: {LMtrack+=1; break;}
         case 2: {LMtrack+=1; break;}
         case 3: {LMtrack=0; break;}
  };
  LightMusicMenu();
}




void LMMenuEnter()
{
    if (LMtrackchosen==0){LMtrackchosen=1;LMswitch=true;LMtrackCurrent=LMtrack;} else if (LMtrackchosen==1){LMtrackchosen=0;LMswitch=false;};
    strip.clear();                          // очистить
    strip.show();  
    LightMusicMenu();
    
}


//...............................................................................
//........................калибровка текущего уровня шума........................
//...............................................................................


void calibration()
{
  Noise=0;
  int CurrentNoise=0;
  for (byte i = 0; i < 150; i ++) // делаем 150 измерений текущего уровня шума 
  {                                
          CurrentNoise = analogRead(A1);
          if (Noise < CurrentNoise) {Noise = CurrentNoise;};   // ищем максимальное
   };
  Freqcalibration();
  }


//...............................................................................
//...................калибровка текущих шумов по частотам........................
//...............................................................................

  void Freqcalibration()
{      
   //наполняем массив калибровки 10ю новыми значениями для всех частот  
 for (byte i=0;i<50;i++)
   {
  for (int j = 0 ; j < FHT_N ;j++) 
    {              
    int sample = analogRead(A1);
    fht_input[j] = sample; // put real data into bins
    };
    fht_window();          // window the data for better frequency response
    fht_reorder();         // reorder the data before doing the fht
    fht_run();             // process the data in the fht
    fht_mag_log();         // take the output of the fht

  for (byte freq = 0; freq < 12; freq++) {   
    AllFreqNoise[freq][i] = fht_log_out[posOffset[freq]];
    };
  };
 
  //Среди частот в наполненных массивах выбираем масксимальные значения, и устанавливаем их, как шумы
  int NoiseMax=0; //переменная для нахождения максимального значения шума
   for (byte freq=0;freq<12;freq++) {
    NoiseMax=0;
    for (byte num=0;num<50;num++)
    { 
      if (AllFreqNoise[freq][num]>NoiseMax) NoiseMax=AllFreqNoise[freq][num];
    };
        FreqNoise[freq]=NoiseMax;
   };
  }


//...............................................................................
//...........................Функция имитации огня...............................
//...............................................................................

//these values are substracetd from the generated values to give a shape to the animation
const unsigned char valueMask[6][6] PROGMEM = {
  {32 , 0  , 0  , 0  , 0  , 0  },
  {64 , 32 , 0  , 0  , 0  , 32 },
  {96 , 64 , 32 , 0  , 32 , 64 },
  {128, 96 , 64 , 32 , 64 , 96 },
  {160, 128, 96 , 64 , 96 , 128},
  {192, 160, 128, 96 , 128, 160}
};

//these are the hues for the fire,
//should be between 0 (red) to about 25 (yellow)
const unsigned char hueMask[6][6] PROGMEM = {
  {1 , 11, 19, 25, 25, 19 }, 
  {1 , 8 , 13, 19, 25, 14 }, 
  {1 , 8 , 12, 16, 19, 12 }, 
  {1 , 5 , 8,  13, 11, 7  }, 
  {1 , 1 , 5,  7,  5,  3  }, 
  {0 , 0 , 0 , 1 , 0 , 1  } 
};

void fireRoutine() {
  static uint32_t prevTime = 0;
  if (millis() - prevTime > 30) {
    prevTime = millis();
    if (pcnt >= 100) {
      shiftUp();
      generateLine();
      pcnt = 0;
    }
    drawFrame(pcnt);
    pcnt += 30;
    strip.show();
  }
}


// Randomly generate the next line (matrix row)

void generateLine() {
  for (uint8_t x = 0; x < M_WIDTH; x++) {
    line[x] = random(64, 255);
  }
}

//shift all values in the matrix up one row

void shiftUp() {
  for (uint8_t y = M_HEIGHT - 1; y > 0; y--) {
    for (uint8_t x = 0; x < M_WIDTH; x++) {
      uint8_t newX = x;
      if (x > 5) newX = x % 15;
      if (y > 5) continue;
      matrixValue[y][newX] = matrixValue[y - 1][newX];
    }
  }

  for (uint8_t x = 0; x < M_WIDTH; x++) {
    uint8_t newX = x;
    if (x > 5) newX = x % 15;
    matrixValue[0][newX] = line[newX];
  }
}

// draw a frame, interpolating between 2 "key frames"
// @param pcnt percentage of interpolation

void drawFrame(int pcnt) {
  int nextv;

  //each row interpolates with the one before it
  for (unsigned char y = M_HEIGHT; y > 0; y--) {
    for (unsigned char x = 0; x < M_WIDTH; x++) {
      uint8_t newX = x;
      if (x > 5) newX = x % 15;
        nextv =
          (((100.0 - pcnt) * matrixValue[y][newX]
            + pcnt * matrixValue[y - 1][newX]) / 100.0)
          - pgm_read_byte(&(valueMask[y][newX]));
            HsvColor hsv;
            hsv.h=HUE_START + pgm_read_byte(&(hueMask[y][newX]));
            hsv.s=255;
            hsv.v=(uint8_t)max(0, nextv);
            HsvToRgb(hsv);
        strip.setPixelColor(x*6+y,ShR,ShG,ShB,ShW);

    }
  }

  //first row interpolates with the "next" line
  for (unsigned char x = 0; x < M_WIDTH; x++) {
    uint8_t newX = x;
    if (x > 5) newX = x % 5;
    HsvColor hsv;
    hsv.h=HUE_START + pgm_read_byte(&(hueMask[0][newX])); // H
    hsv.s=255;           // S
    hsv.v=(uint8_t)(((100.0 - pcnt) * matrixValue[0][newX] + pcnt * line[newX]) / 100.0); // V
    HsvToRgb(hsv);
    strip.setPixelColor(x,ShR,ShG,ShB,ShW);
                    
  }
}


pixel pixelfading(pixel one)
{
    float RR,GG,BB,WW;
    RR=one.r/ConstFadingPix; 
    GG=one.g/ConstFadingPix; 
    BB=one.b/ConstFadingPix; 
    WW=one.w/ConstFadingPix;
    
     if (one.w!=0)
       {
        one.r-=ceil(RR);
        one.g-=ceil(GG);
        one.b-=ceil(BB);
        one.w-=ceil(WW);
        if (RR+GG+BB+WW<(one.r+one.g+one.b+one.w)/(3*ConstFadingPix) || RR+GG+BB+WW==0) {one.r=0;one.g=0;one.b=0;one.w=0;return one;};
       }
     else 
       {
         one.r-=ceil(RR);
         one.g-=ceil(GG);
         one.b-=ceil(BB);
         if (RR+GG+BB<(one.r+one.g+one.b)/(3*ConstFadingPix) || RR+GG+BB+WW==0) {one.r=0;one.g=0;one.b=0;return one;};
       };
       
   return one;
}

pixel colcalibr(bool half, byte i, pixel one)
{
  if (half==false)
  {
switch (i)
{
  case 0: { one.r=100;    one.g=0;    one.b=250;  one.w=0;  break;}
  case 1: { one.r=0;      one.g=0;    one.b=250;  one.w=0;  break;}
  case 2: { one.r=0;      one.g=150;  one.b=250;  one.w=0;  break;}
  case 3: { one.r=0;      one.g=250;  one.b=0;    one.w=0;  break;}
  case 4: { one.r=250;    one.g=150;  one.b=0;    one.w=0;  break;}
  case 5: { one.r=250;    one.g=0;    one.b=0;    one.w=0;  break;}
};
  }
else 
 {
  switch (i)
  {
  case 0: { one.r=50;   one.g=0;    one.b=125;  one.w=0;  break;}
  case 1: { one.r=0;    one.g=0;    one.b=125;  one.w=0;  break;}
  case 2: { one.r=0;    one.g=75;   one.b=125;  one.w=0;  break;}
  case 3: { one.r=0;    one.g=125;  one.b=0;    one.w=0;  break;}
  case 4: { one.r=125;  one.g=75;   one.b=0;    one.w=0;  break;}
  case 5: { one.r=125;  one.g=0;    one.b=0;    one.w=0;  break;}
  };
 };
 return one;
}
