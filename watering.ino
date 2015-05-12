//Digital pins
int photoResistorPowerSource = 2;
int soilHumidityPowerSource = 3;
//L298N relevant
int l298NPowerSource = 6;
int motorValveIN1 = 7;
int motorValveIN2 = 8;
//Analog pins
int photoResistor = 0;  //Connected to sensor's output and grounded via 10k resistor
int soilHumidity = 1;  //Connected to sensor's output and grounded via 10k resistor

//Configuration variables
int nightLightLevel = 700;
int soilHumidityLowLevel = 800;
unsigned long measurementInterval = 2000;  //Overall measurement interval
unsigned long capacitanceDelay = 500;  //Delay after sourcing power to sensors prior to measurement
unsigned long sensorsMeasurementInterval = 500;  //Interval between measurements of each sensor
int brightnessSeriesAssuringNight = 3;  //Number of contiguous measurements of brightness below threshold assuring night
int humiditySeriesAssuringSoilIsDry = 3;  //Number of contiguous measurements of soil humidity below threshold assuring watering needed
unsigned long wateringDuration = 10000;
unsigned long wateringHoldTime = 20000;  //Minimum time interval between waterings
unsigned long motorValveRunningDuration = 4000;  //Time needed to open or close valve
int maxWateringsPerNight = 3;
//End of configuration variables

int measuredSoilHumidityLevel = 0;
int measuredLightLevel = 0;
unsigned long lastMeasurementStartTime = 0;
unsigned long lastWateringStartTime = 0;
int measurementStage = 0;
int matchingLightSeries = 0;
int matchingSoilHumiditySeries = 0;
int wateringsLastNight = 0;
boolean valveOpened = true;
boolean motorValveIsRunning = false;
unsigned long motorValveStartTime = 0;

void setup() {
  pinMode(photoResistorPowerSource, OUTPUT);
  pinMode(soilHumidityPowerSource, OUTPUT);
  pinMode(l298NPowerSource, OUTPUT);
  pinMode(motorValveIN1, OUTPUT);
  pinMode(motorValveIN2, OUTPUT);
  digitalWrite(soilHumidityPowerSource, LOW);
  digitalWrite(photoResistorPowerSource, LOW);
  maxWateringsPerNight--;  //Because counter starts from 0

  Serial.begin(9600);

}

void loop() {

  // Stop motor valve if it's running enough
  if (motorValveIsRunning && ((millis() - motorValveStartTime) >= motorValveRunningDuration)) {
    Serial.println("\tSwitching off motor valve");
    digitalWrite(l298NPowerSource, LOW);
    digitalWrite(motorValveIN1, LOW);
    digitalWrite(motorValveIN2, LOW);
    motorValveIsRunning = false;
  }

  switch (measurementStage) {
  case 0:
    // Start powering sensors
    if ((millis() - lastMeasurementStartTime) >= measurementInterval) {
        lastMeasurementStartTime = millis();
        measurementStage++;
        digitalWrite(photoResistorPowerSource, HIGH);
        digitalWrite(soilHumidityPowerSource, HIGH);
      }
      break;
    case 1:
      // Wait little more time for sensors's capacitance to be set and start measuring light level
      if ((millis() - lastMeasurementStartTime) >= capacitanceDelay) {
        measuredLightLevel = analogRead(photoResistor);

        // Check if night is really coming by counting repetitive results below threshold
        if (measuredLightLevel < nightLightLevel) {
          matchingLightSeries++;
        }
        else {
          matchingLightSeries = 0;

          // Reset last night waterings counter
          wateringsLastNight = 0;
        }

        measurementStage++;
      }
      break;
    case 2:
      // Start measuring soil humidity level
      if ((millis() - lastMeasurementStartTime) >= (capacitanceDelay + sensorsMeasurementInterval)) {
        measuredSoilHumidityLevel = analogRead(soilHumidity);

        // Check if soil is really dry by counting repetitive results below threshold
        if (measuredSoilHumidityLevel < soilHumidityLowLevel) {
          matchingSoilHumiditySeries++;
        }
        else {
          matchingSoilHumiditySeries = 0;
        }

        measurementStage++;

        // Print some useful info
        Serial.print("Light: ");
        Serial.print(measuredLightLevel);
        Serial.print("\tNightfall? ");
        if (matchingLightSeries >= brightnessSeriesAssuringNight) {
          Serial.print("Yes. ");
          Serial.print(wateringsLastNight);
          Serial.println(" waterings this night");
        }
        else Serial.println("No");
        Serial.print("Soil humidity: ");
        Serial.println(measuredSoilHumidityLevel);
      }
      break;
    case 3:
      // Power off sensors
      digitalWrite(photoResistorPowerSource, LOW);
      digitalWrite(soilHumidityPowerSource, LOW);
      measurementStage = 0;
  }

  // Start watering
  if (((millis() - lastWateringStartTime) >= (wateringDuration + wateringHoldTime)) && \
      (motorValveIsRunning == false) && \
      (wateringsLastNight <= maxWateringsPerNight) && \
      (matchingLightSeries >= brightnessSeriesAssuringNight) \
      /*&& (matchingSoilHumiditySeries >= humiditySeriesAssuringSoilIsDry)*/) {
    lastWateringStartTime = millis();
    digitalWrite(l298NPowerSource, HIGH);
    digitalWrite(motorValveIN1, HIGH);
    digitalWrite(motorValveIN2, LOW);
    motorValveStartTime = millis();
    valveOpened = true;
    motorValveIsRunning = true;
    Serial.println("\tStart watering. Opening valve");
  }

  // Stop watering
  if (valveOpened && ((millis() - lastWateringStartTime) >= wateringDuration)) {
    digitalWrite(l298NPowerSource, HIGH);
    digitalWrite(motorValveIN1, LOW);
    digitalWrite(motorValveIN2, HIGH);
    wateringsLastNight++;
    motorValveStartTime = millis();
    valveOpened = false;
    motorValveIsRunning = true;
    Serial.println("\tStop watering. Closing valve");
  }

}
