#define F_CPU 1000000UL
#include <util/delay.h>
const int SHUTDOWN_PIN = 3;
const int RELAY_PIN = 0;
const int LED_PIN = 1;

int checkOffCount = 0;
int waitingOffTimeoutCount=0;
volatile int powerstatus = 0; 
//Power Toggle Mode
// 0 Power off
// 2 Powered on 
// 3 Powering off
// 4 Waiting for Safe Shutdown to finish
// 5 Hard Shutdown

volatile int NextBit;
volatile unsigned long RecdData;

void ReceivedCode(boolean Repeat) {
  // Check for correct remote control
  if ((RecdData & 0xFFFF) != 0xef00) return;
  int key = RecdData>>16 & 0xFF;

  if(key==3 && !Repeat)
  {    
    if(powerstatus==0)
      powerstatus=2;     
  }
  else if(key==2 && !Repeat)
  {
    if(powerstatus==2)
      powerstatus=3; 
  }


}

// Called on falling PB2
ISR(INT0_vect) {
  int timet = TCNT0;
  int overflowt = TIFR & 1<<TOV0;
  
  if (NextBit == 32) {
    if ((timet >= 194) && (timet <= 228) && (overflowt == 0)) {
      RecdData = 0; NextBit = 0;
    } else if ((timet >= 159) && (timet <= 193) && (overflowt == 0)) ReceivedCode(1);
  // Data bit
  } else {
    if ((timet > 44) || (overflowt != 0)) NextBit = 32; // Invalid - restart
    else {
      if (timet > 26) RecdData = RecdData | ((unsigned long) 1<<NextBit);
      if (NextBit == 31) ReceivedCode(0);
      NextBit++;
    }
  }
  TCNT0 = 0;                  //Reset counter
  TIFR = TIFR | 1<<TOV0;      //Reset overflow
  GIFR = GIFR | 1<<INTF0;     //Reset INT0
}

void setup() {
  // put your setup code here, to run once:

    pinMode(5, INPUT); //LED 
    pinMode(LED_PIN, OUTPUT); //LED 
    pinMode(RELAY_PIN, OUTPUT); //Power Relay
    pinMode(SHUTDOWN_PIN, OUTPUT); //Shutdown trigger

    digitalWrite(LED_PIN, HIGH);
    _delay_ms(200);
    digitalWrite(LED_PIN, LOW);
    _delay_ms(200);
    digitalWrite(LED_PIN, HIGH);
    _delay_ms(200);
    digitalWrite(LED_PIN, LOW);

    //Set up timer, assume 1Mhz
    TCCR0A = 0;                 
    TCCR0B = 3<<CS00;           
    
    MCUCR = MCUCR | 2<<ISC00;   
    GIMSK = GIMSK | 1<<INT0;    
    NextBit = 32;               
}





bool checkPiOff()
{
   int sensorValue = analogRead(2); //P4 read input GPIO14
   return ((sensorValue/1024.0) < 0.3);
}

const bool useIR = true;




void loop() {
      // put your main code here, to run repeatedly:
      
      int buttonread = analogRead(0);
      //Using Analog input to detect button because all other pin are used.
      if(buttonread < 750)
      {
          if(powerstatus==0)
            powerstatus=2;
          else if(powerstatus==2)
            powerstatus=3; 
      }
    
      if(powerstatus==2)
      {
          digitalWrite(SHUTDOWN_PIN, LOW);
          digitalWrite(LED_PIN, HIGH);
          digitalWrite(RELAY_PIN, HIGH);

          _delay_ms(3000);                
      }
      else if(powerstatus==3)
      {
          //Start Shutdown Sequence
          // Call 
          digitalWrite(LED_PIN, LOW);
          digitalWrite(SHUTDOWN_PIN, HIGH);
          _delay_ms(300);
          digitalWrite(SHUTDOWN_PIN, LOW);
          powerstatus=4;
          checkOffCount = 0;
          waitingOffTimeoutCount = 0;
      }
      else if(powerstatus==4)
      {
        
          digitalWrite(LED_PIN, HIGH);
          _delay_ms(200);
          digitalWrite(LED_PIN, LOW);
          _delay_ms(200);
          
          //Check if it's off 5 times before shutting down                
          if(checkPiOff())
            checkOffCount = checkOffCount + 1;
          else
          {
            checkOffCount=0;
            waitingOffTimeoutCount = waitingOffTimeoutCount + 1;
          }
          
          if(checkOffCount>=2)
          {
              //It's really off
              digitalWrite(LED_PIN, LOW);
              digitalWrite(RELAY_PIN, LOW); 
              powerstatus = 0;
              checkOffCount = 0;
              waitingOffTimeoutCount = 0; 
          }
          else if(waitingOffTimeoutCount>=20)
          {
            //Time out shutdown anyway
            digitalWrite(LED_PIN, LOW);
            digitalWrite(RELAY_PIN, LOW); 
            powerstatus = 0;
            checkOffCount = 0;
            waitingOffTimeoutCount = 0;                        
          }
          
      }
      else if(powerstatus==5) //Hard shutdown
      {
          digitalWrite(LED_PIN, LOW);
          digitalWrite(RELAY_PIN, LOW);
          powerstatus = 0;
          checkOffCount = 0;
      }

      
}

