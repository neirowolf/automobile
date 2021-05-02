#include <EEPROM.h>


#include "GyverButton.h"

#include <TM1637.h>
#define CLK A4
#define DIO A5

#include <SoftwareSerial.h>
#define SIM_RX D4
#define SIM_TX D5

SoftwareSerial SIM800(SIM_RX, SIM_TX);


#define RTIME_05 300000 // Время поездки 5 минут
#define RTIME_07 420000 // Время поездки 7 минут
#define RTIME_10 600000 // Время поездки 10 минут

#define TIMER_PIN 7 //переключение таймера  счетчиков 5-7-10 минут
#define SCREEN_PIN 6 //сингнал с кнопки чтения счетчиков 5-7-10 минут


TM1637 disp(CLK, DIO);

int pinTouch = 8;		// времено неиспользется
int pinTouchRele = 6;	// исходящий сигнал включается после выбора режима таймера
int pinRide = A3;		//не используется

int keyPin=A1;			//переключение таймера  счетчиков 5-7-10 минут
GButton keyBTN(TIMER_PIN);
int keyPin1=A2;			//сингнал с кнопки чтения счетчиков 5-7-10 минут
GButton screenBTN(SCREEN_PIN);
unsigned long keyTrashhold; 

unsigned long workTimer;

boolean isRun;

unsigned long timerDelay;
unsigned long runTime;
unsigned long displayUpdateTime;

unsigned long rideTrashhold;
unsigned long startPressing; //Начало нажатия кнопки
unsigned long endPressing; //Конец нажатия кнопки

byte rideN_05; // количество поездок 5 минут
byte rideN_07; // количество поездок 7 минут
byte rideN_10; // количество поездок 10 минут
byte k; //Выводимый счётчик

bool isShowRide=true;//Показывать счётчик поездок


// Вычисляет время запуска события.
// Получает время, через которое наступит событие (в милисекундах)
// Возвращает время старта
unsigned long getTimeLine(unsigned long time)
{
  return millis() + time;
}

// Устанавливает время ожидания для устранения дребезга
unsigned long getTrashhold()
{
  return getTimeLine(50);
}

// Определяет, подошло ли время события
boolean isTimeCome(unsigned long mark)
{
  return (mark >0)&&(mark < millis()); // Если метка определена и текущее время больше метки
}

void setup() {
  SIM800.begin(9600);
  Serial.begin(9600);
  pinMode(pinTouch, INPUT_PULLUP);
  pinMode(pinRide, INPUT_PULLUP);
 // pinMode(keyPin, INPUT_PULLUP);
 // pinMode(keyPin1, INPUT_PULLUP);
  pinMode(pinTouchRele, OUTPUT);
  
  
  
  //Устанавливаем режим кнопок
  keyBTN.setType(HIGH_PULL);
  keyBTN.setDirection(NORM_OPEN);
  screenBTN.setType(HIGH_PULL);
  screenBTN.setDirection(NORM_OPEN);
  
  //////////////////////////////


 keyTrashhold=1;
 rideTrashhold=1;
 
 workTimer=0;
 isRun=false;
 timerDelay=0;

 disp.init();  // инициализация
 disp.set(7);


 rideN_05=EEPROM.read(0);
 rideN_07=EEPROM.read(1);
 rideN_10=EEPROM.read(2);
// if(rideN==255){rideN=0;EEPROM.update(0, rideN);}

 startPressing=endPressing=0;
 
}

void loop()
{
	keyBTN.tick();
	screenBTN.tick();
	
	if(isTimeCome(timerDelay))//Если прошла задержка перед стартом
	{
		if(!isRun)//Если ещё не едем, 
		{
			isRun=true;//запускаем поездку
			runTime=getTimeLine(workTimer);// Устанавливаем время поездки
			saveRide();// и увеличиваем счётчик поездок
		}
	}
	else
	{
		digitalWrite(pinTouchRele, digitalRead(pinTouch));
		keyButton();
		showRide();
	}
  
	if(isRun)
	{
		if(isTimeCome(runTime))
		{
			isRun=false;
			timerDelay=0;
			workTimer=0;
			runTime=0;
		}
		else
		{
			digitalWrite(pinTouchRele, LOW);
			printTimer(runTime-millis());
		}
	}
}

void SIMCommand(String s)
{
	SIM800.print(s);
	SIM800.print(13);
	
	Serial.println(SIMAnswer());
}

String SIMAnswer() {  //функция чтения данных от GSM модуля
  int c;
  String v;
  while (SIM800.available()) {  //сохраняем входную строку в переменную v
    c = SIM800.read();
    v += char(c);
    delay(10);
  }
  return v;
}


// Отправляем данные через SIM
void sendSIMData()
{
	String msgString;
	byte msg[4];
	msg[0]=rideN_05;
	msg[1]=rideN_07;
	msg[2]=rideN_10;
	msg[2]=0x1A;
	
	msgString = String((char*)msg);
	
	SIMCommand("AT+CPIN?");
	SIMCommand("AT+CSQ");
	SIMCommand("AT+CREG?");
	SIMCommand("AT+CGATT?");
	SIMCommand("AT+CIPMODE=0");
	SIMCommand("AT+CIPMUX=0");
	
	SIMCommand("AT+CSTT=\"internet\"");
	SIMCommand("AT+CIPSTATUS");
	SIMCommand("AT+CIICR");
	SIMCommand("AT+CIPSTATUS");
	SIMCommand("AT+CIFSR");
	SIMCommand("AT+CIPSTATUS");
	SIMCommand("AT+CIPSTART=\"TCP\",\"127.0.0.1\",80");
	SIMCommand("AT+CIPSTATUS");
	
	SIMCommand("AT+CIPSEND?");
	SIMCommand("AT+CIPQSEND?");
	SIMCommand("AT+CIPSEND=4");
	
	SIMCommand(msgString);
	
	SIMCommand("AT+CIPCLOSE");	
}

void saveRide()
{
 switch(workTimer)
 {
	case(RTIME_05):{rideN_05=updateRideLine(rideN_05,0);}break;
	case(RTIME_07):{rideN_07=updateRideLine(rideN_07,1);}break;
	case(RTIME_10):{rideN_10=updateRideLine(rideN_10,2);}break;
 }
 sendSIMData();
}



byte updateRideLine(byte ride, int n)
{
 if(ride==255){ride=0;}
 ride++;
 
 EEPROM.update(n, ride);
 return ride;
}

// Вывести время на дисплей
void printTimer(unsigned long runTime)
{
  int8_t TIME[4];
  int8_t mins;
  int8_t sec;

  unsigned long rest;
  
  if(displayUpdateTime==0){displayUpdateTime=1;}
  if(isTimeCome(displayUpdateTime))
  {
    
    rest=runTime/1000;
  
    mins=rest/60;
    sec=rest%60;
  
    TIME[0] = byte(mins / 10);           // получить десятки минут
    TIME[1] = byte(mins % 10);           // получить единицы минут
    TIME[2] = byte(sec / 10);            // получить десятки секунд
    TIME[3] = byte(sec % 10);            // получить единицы секунд

    if(sec&1){disp.point(0);}// Если секунда нечётная скрываем двоеточие
    else{disp.point(1);}// Если чётная - показываем
    
    disp.display(TIME);
    displayUpdateTime=getTimeLine(500);
  }
}

//Показываем количество поездок
void showRide()
{
  //if(digitalRead(keyPin1)==LOW)
  if(isShowRide)
  {
	  switch(k)
	  {
		  case(0):{printRide(k,rideN_05);}break;
		  case(1):{printRide(k,rideN_07);}break;
		  case(2):{printRide(k,rideN_10);}break;
	  }
  }
}

// Выводим на экран количество поезок
void printRide(byte t,byte ride)
{
  int8_t TIME[4];
	
	switch(t)// индекс счётчика
	{
		case(0):{TIME[0] = 5;}break;
		case(1):{TIME[0] = 7;}break;
		case(2):{TIME[0] = 1;}break;
	}
    			
    TIME[1] = ride/100;     // получить сотни
    TIME[2] = (ride/10)%10; // получить десятки
    TIME[3] = ride%10;      // получить единицы
    disp.point(0);
    
    disp.display(TIME);
}

void keyButton()
{
  if(screenBTN.isHold()&&screenBTN.getHoldClicks()>3000)
  {
	workTimer=10000;
	printTimer(workTimer);
	keyTrashhold=getTrashhold();
				  
	timerDelay=getTimeLine(10);
  }
  
  if(screenBTN.isClick())
  {
	  isShowRide=true;
		k++;
		if(k>2){k=0;}
  }
	
  if(keyBTN.isClick())
  {
	  switch(workTimer)
	  {
		  case(RTIME_05):
		  { workTimer=RTIME_07; }break;
		  case(RTIME_07):
		  { workTimer=RTIME_10; }break;
		  case(RTIME_10):
		  { workTimer=0; }break;
		  default:{workTimer=RTIME_05;}
		  }
		  isShowRide=false;
		  printTimer(workTimer);
		  keyTrashhold=getTrashhold();
		  
		  timerDelay=getTimeLine(3000);
  }
}
