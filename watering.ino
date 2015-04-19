//Digital pins
int photoResistorPowerSource = 2;
int soilHumidityPowerSource = 3;
//Analog pins
int photoResistor = 0;
int soilHumidity = 1;

int nightBrightnessLevel = 800;
int soilHumidityLowLevel = 700;
unsigned long measurementInterval = 2000;  //Overall measurement interval
unsigned long capacitanceDelay = 500;  //Delay after sourcing power to sensors prior to measurement
unsigned long interSensorMeasurementInterval = 500;  //Interval between measurements of each sensor
int brightnessSeriesAssuringNight = 3;  //Number of contiguous measurements of brightness below threshold assuring night
int soilHumiditySeriesAssuringValveOpening = 3;  //Number of contiguous measurements of soil humidity below threshold assuring valve opening
unsigned long wateringDuration = 10000;
unsigned long wateringHoldTime = 20000;  //Minimum time interval between waterings
unsigned long valveMotorRunningDuration = 4000;

int measuredSoilHumidityLevel = 0;
int measuredBrightnessLevel = 0;
unsigned long lastMeasurementStartTime = 0;
unsigned long lastWateringStartTime = 0;
int matchingBrightnessSeries = 0;
int matchingSoilHumiditySeries = 0;
boolean valveOpened = false;
boolean valveMotorIsRunning = true;
unsigned long valveMotorStartTime = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(photoResistorPowerSource, OUTPUT);
  digitalWrite(photoResistorPowerSource, LOW);
  pinMode(soilHumidityPowerSource, OUTPUT);
  digitalWrite(soilHumidityPowerSource, LOW);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);

  digitalWrite(7, LOW);
  digitalWrite(8, HIGH);
  delay(valveMotorRunningDuration);

  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:

  if (valveMotorIsRunning && ((millis() - valveMotorStartTime) >= valveMotorRunningDuration)) {
    digitalWrite(7, LOW);
    digitalWrite(8, LOW);
    valveMotorIsRunning = false;
  }

  if ((millis() - lastMeasurementStartTime) >= measurementInterval) {
    lastMeasurementStartTime = millis();
    digitalWrite(photoResistorPowerSource, HIGH);
    digitalWrite(soilHumidityPowerSource, HIGH);
    delay(capacitanceDelay);

    measuredBrightnessLevel = analogRead(photoResistor);
    Serial.print("Light: ");
    Serial.print(measuredBrightnessLevel);
    if (measuredBrightnessLevel < nightBrightnessLevel) {
      matchingBrightnessSeries++;
    }
    else {
      matchingBrightnessSeries = 0;
    }
    Serial.print("\t");
    Serial.print(matchingBrightnessSeries);
    delay(interSensorMeasurementInterval);

    measuredSoilHumidityLevel = analogRead(soilHumidity);
    Serial.print("\tSoil humidity: ");
    Serial.print(measuredSoilHumidityLevel);
    Serial.print("\n");
    if (measuredSoilHumidityLevel < soilHumidityLowLevel) {
      matchingSoilHumiditySeries++;
    }
    else {
      matchingSoilHumiditySeries = 0;
    }

    digitalWrite(photoResistorPowerSource, LOW);
    digitalWrite(soilHumidityPowerSource, LOW);
  }

//  if ((matchingBrightnessSeries == brightnessSeriesAssuringNight) && (matchingSoilHumiditySeries == soilHumiditySeriesAssuringValveOpening) && \
//      ((millis() - lastWateringStartTime) >= (wateringDuration + wateringHoldTime))) {
  if ((matchingBrightnessSeries == brightnessSeriesAssuringNight) && \
      ((millis() - lastWateringStartTime) >= (wateringDuration + wateringHoldTime))) {        
    lastWateringStartTime = millis();
    digitalWrite(7, HIGH);
    digitalWrite(8, LOW);
    valveMotorStartTime = millis();
    valveOpened = true;
    valveMotorIsRunning = true;
  }

  if (valveOpened && ((millis() - lastWateringStartTime) >= wateringDuration)) {
    digitalWrite(7, LOW);
    digitalWrite(8, HIGH);
    valveMotorStartTime = millis();
    valveOpened = false;
    valveMotorIsRunning = true;
  }



}
