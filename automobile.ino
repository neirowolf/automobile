#include <EEPROM.h>

#include <TM1637.h>
#define CLK 3
#define DIO 2

#define RTIME_05 300000 // Время поездки 5 минут
#define RTIME_07 420000 // Время поездки 7 минут
#define RTIME_10 600000 // Время поездки 10 минут


TM1637 disp(CLK, DIO);

int pinTouch = 8;		// времено неиспользется
int pinTouchRele = 4;	// исходящий сигнал включается после выбора режима таймера
int pinRide = 5;		//не используется

int keyPin=6;			//переключение таймера  счетчиков 5-7-10 минут
int keyPin1=7;			//сингнал с кнопки чтения счетчиков 5-7-10 минут
int keyState;
unsigned long keyTrashhold; 

unsigned long workTimer;

boolean isRun;

unsigned long timerDelay;
unsigned long runTime;
unsigned long displayUpdateTime;

unsigned long rideTrashhold;

byte rideN_05; // количество поездок 5 минут
byte rideN_07; // количество поездок 7 минут
byte rideN_10; // количество поездок 10 минут
byte k; //Выводимый счётчик

int rideState;

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
Serial.begin(9600);
  pinMode(pinTouch, INPUT_PULLUP);
  pinMode(pinRide, INPUT_PULLUP);
  pinMode(keyPin, INPUT_PULLUP);
  pinMode(keyPin1, INPUT_PULLUP);
  pinMode(pinTouchRele, OUTPUT);


 keyTrashhold=1;
 rideTrashhold=1;
 
 keyState=0;
 workTimer=0;
 isRun=false;
 timerDelay=0;

 disp.init();  // инициализация
 disp.set(7);


 rideN_05=EEPROM.read(0);
 rideN_07=EEPROM.read(1);
 rideN_10=EEPROM.read(2);
// if(rideN==255){rideN=0;EEPROM.update(0, rideN);}
 
}

void loop()
{
  if(isTimeCome(timerDelay))
  {
    if(!isRun)
    {
      isRun=true;
      runTime=getTimeLine(workTimer);		
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
	  saveRide();
		
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

void saveRide()
{
 switch(workTimer)
 {
	case(RTIME_05):{rideN_05=updateRideLine(rideN_05,0)}break;
	case(RTIME_07):{rideN_07=updateRideLine(rideN_07,1)}break;
	case(RTIME_10):{rideN_10=updateRideLine(rideN_10,2)}break;
 }
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
  if(digitalRead(keyPin1)==LOW)
  {
	  swithc(k):
	  {
		  case(0){printRide(k,rideN_05);}break;
		  case(1){printRide(k,rideN_07);}break;
		  case(2){printRide(k,rideN_10);}break;
	  }
  }
}

// Выводим на экран количество поезок
void printRide(byte t,byte ride)
{
  int8_t TIME[4];

  
  
    rest=runTime/1000;
  
    mins=rest/60;
    sec=rest%60;
  
    TIME[0] = t;			// индекс счётчика
    TIME[1] = ride/100;     // получить сотни
    TIME[2] = (ride/10)%10; // получить десятки
    TIME[3] = ride%10;      // получить единицы
    disp.point(0);
    
    disp.display(TIME);
}

void keyButton()
{
  if(isTimeCome(rideTrashhold))
  {
    if(digitalRead(keyPin1)==HIGH)
	{
		k++;
		if(k>2){k=0;}
		rideTrashhold=getTrashhold();
	}
  }
	
  if(isTimeCome(keyTrashhold))
  {
    
    switch(keyState)
    {
      case(0):
      {
        if(digitalRead(keyPin)==HIGH)
        {
         keyTrashhold=getTrashhold();
         keyState=1;
        }
      }break;
      case(1):
      {
       if(digitalRead(keyPin)==LOW)
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
          printTimer(workTimer);
          keyTrashhold=getTrashhold();
          keyState=0;
          rideState=0;
          
          timerDelay=getTimeLine(3000);
       }
      }break;
    }
  }
}

