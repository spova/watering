/*I'm using power-saving code from Donal Morrissey's Sleeping Arduino article: http://donalmorrissey.blogspot.ru/2010/04/putting-arduino-diecimila-to-sleep-part.html
He is explaining waking up from watchdog timer the best*/

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

//Pins
int soilMoistureSensorOut = 1;
int photoResistorOut = 0;
int soilMoistureSensorFeed = 2;  //Feeded through 10K resistor
int photoResistorFeed = 4;  //Feeded through 10K resistor
int l298nEnable = 3;
int l298nIN3 = 8;
int l298nIN4 = 7;

//Config
int measurementInterval = 10;
int wateringDuration = 150;
int soilDryLevel = 400;
int nightLightLevel = 200;

boolean sleepAllowed = true;
boolean measurementAllowed = true;
boolean wateringNeeded = false;
boolean alreadyWatered = false;
int measurementCounter = 0;
int wateringCounter = 0;
int wateringTimer = 0;
int soilMoisture;
int lightLevel;
int lowMoistureCounter = 0;
int lowLightCounter = 0;


/***************************************************
 *  Name:        ISR(WDT_vect)
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Watchdog Interrupt Service. This
 *               is executed when watchdog timed out.
 *
 ***************************************************/
ISR(WDT_vect) {}


/***************************************************
 *  Name:        enterSleep
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Enters the arduino into sleep mode.
 *
 ***************************************************/
void enterSleep(void)
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
  sleep_enable();

  /* Now enter sleep mode. */
  sleep_mode();

  /* The program will continue from here after the WDT timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */

  /* Re-enable the peripherals. */
  power_all_enable();
}



/***************************************************
 *  Name:        setup
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Setup for the serial comms and the
 *                Watch dog timeout.
 *
 ***************************************************/
void setup()
{
  Serial.begin(9600);

  /*** Setup the WDT ***/

  /* Clear the reset flag. */
  MCUSR &= ~(1 << WDRF);

  /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1 << WDCE) | (1 << WDE);

  /* set new watchdog timeout prescaler value */
  WDTCSR = 1 << WDP0 | 1 << WDP3; /* 8.0 seconds */

  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);

  pinMode(soilMoistureSensorFeed, OUTPUT);
  pinMode(photoResistorFeed, OUTPUT);
  pinMode(l298nEnable, OUTPUT);
  pinMode(l298nIN3, OUTPUT);
  pinMode(l298nIN4, OUTPUT);
}



void loop()
{
  /*Serial.println(measurementCounter);
  delay(100);*/

  if (measurementAllowed) {
    if (measurementCounter >= measurementInterval) {
      digitalWrite(soilMoistureSensorFeed, HIGH);
      delay(500);
      soilMoisture = analogRead(soilMoistureSensorOut);
      digitalWrite(soilMoistureSensorFeed, LOW);
      if (soilMoisture >= soilDryLevel) {
        lowMoistureCounter++;
      }
      else {
        lowMoistureCounter = 0;
      }

      digitalWrite(photoResistorFeed, HIGH);
      delay(500);
      lightLevel = analogRead(photoResistorOut);
      digitalWrite(photoResistorFeed, LOW);
      if (lightLevel >= nightLightLevel) {
        lowLightCounter++;
      }
      else {
        lowLightCounter = 0;
        alreadyWatered = false;
      }

      if ((lowMoistureCounter >= 3) && (lowLightCounter >= 3)) {
        Serial.println("Watering needed");
        wateringNeeded = true;
      }
      else {
        wateringNeeded = false;
      }

      measurementCounter = 0;

      Serial.print("Soil moisture:\t");
      Serial.println(soilMoisture);
      Serial.print("Light level:\t");
      Serial.println(lightLevel);
      Serial.print("Waterings:\t");
      Serial.println(wateringCounter);
      delay(100);
    }
    else {
      measurementCounter++;
    }
  }

  if ((wateringNeeded) && !(alreadyWatered)) {
    if (wateringTimer == 0) {
      //Serial.println("Start watering");
      digitalWrite(l298nEnable, HIGH);
      digitalWrite(l298nIN3, HIGH);
      digitalWrite(l298nIN4, LOW);
      delay(5000);
      digitalWrite(l298nEnable, LOW);
      digitalWrite(l298nIN3, LOW);
      digitalWrite(l298nIN4, LOW);
      measurementAllowed = false;
    }

    wateringTimer++;

    if (wateringTimer >= wateringDuration) {
      //Serial.println("Stop watering");
      digitalWrite(l298nEnable, HIGH);
      digitalWrite(l298nIN3, LOW);
      digitalWrite(l298nIN4, HIGH);
      delay(5000);
      digitalWrite(l298nEnable, LOW);
      digitalWrite(l298nIN3, LOW);
      digitalWrite(l298nIN4, LOW);
      measurementAllowed = true;
      wateringTimer = 0;
      wateringCounter++;
      alreadyWatered = true;
    }
  }

  enterSleep();
}
