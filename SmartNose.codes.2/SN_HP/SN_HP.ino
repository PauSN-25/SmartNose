#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"


// === Pines SPI y CS de los sensores ===
#define SCK_PIN 12
#define MOSI_PIN 11
#define MISO_PIN 10
#define CS_A 8
#define CS_B 7
#define CS_C 6


// === Ventilador ===
#define FAN_PIN 2
#define FAN_CHANNEL 0


// === Sensores BME688 SPI ===
Adafruit_BME680 bme_A(CS_A);
Adafruit_BME680 bme_B(CS_B);
Adafruit_BME680 bme_C(CS_C);


// === HP-A ===
const int HP_A_STEPS = 6;
const int HP_A[HP_A_STEPS][2] = {
  {100, 6}, {320, 1}, {170, 6}, {320, 1}, {230, 7}, {320, 5}
};
const int HP_A_MEASURE_COUNT = 10;
const int HP_A_MEASURE_TIMES[HP_A_MEASURE_COUNT] = {6,7,12,13,14,17,19,20,23,25};


// === HP-B ===
const int HP_B_STEPS = 4;
const int HP_B[HP_B_STEPS][2] = {
  {80, 8}, {350, 1}, {200, 9}, {350, 8}
};
const int HP_B_MEASURE_COUNT = 10;
const int HP_B_MEASURE_TIMES[HP_B_MEASURE_COUNT] = {7,8,9,10,14,17,19,21,23,25};


// === HP-C ===
const int HP_C_STEPS = 8;
const int HP_C[HP_C_STEPS][2] = {
  {210, 4}, {280, 3}, {350, 3}, {280, 3},
  {210, 3}, {140, 4}, {80, 3}, {140, 3}
};
const int HP_C_MEASURE_COUNT = 10;
const int HP_C_MEASURE_TIMES[HP_C_MEASURE_COUNT] = {3,5,6,7,10,12,15,19,22,25};


// === Variables ===
String ensayoNombre = "";
String tipoMuestra = "";
String camaraEstado = "";
bool corriendo = false;
unsigned int ensayoDuracion = 0;
int fanPorcentaje = 0;


unsigned long lastSensorMillis = 0;
unsigned long lastReadyMillis = 0;
unsigned long startMillis_A = 0, startMillis_B = 0, startMillis_C = 0;
const unsigned long readyInterval = 1000;
const unsigned long sensorInterval = 100;


// === Arrays de gas ===
float gasA_values[10];
float gasB_values[10];
float gasC_values[10];


// Variables de estado para mediciones no bloqueantes
enum MeasureState { IDLE, MEASURING };
MeasureState state_A = IDLE, state_B = IDLE, state_C = IDLE;
unsigned long measureStart_A = 0, measureStart_B = 0, measureStart_C = 0;
const unsigned long measureTimeout = 5000;


// === Funciones ===
void configurarBME(Adafruit_BME680 &bme) {
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(0, 0);
}


int getProfileTemperature(const int perfil[][2], int steps, int tiempoSegundos) {
  int duracionTotal = 0;
  for (int i = 0; i < steps; i++) duracionTotal += perfil[i][1];
  int tiempoCiclo = tiempoSegundos % duracionTotal;
  int acumulado = 0;
  for (int i = 0; i < steps; i++) {
    acumulado += perfil[i][1];
    if (tiempoCiclo < acumulado) return perfil[i][0];
  }
  return perfil[steps - 1][0];
}


int getIndiceMedida(int tiempoActual, const int tiemposMedida[], int num) {
  for (int i = 0; i < num; i++) {
    if (tiempoActual == tiemposMedida[i]) return i;
  }
  return -1;
}


void resetGasArrays() {
  for (int i = 0; i < 10; i++) {
    gasA_values[i] = NAN;
    gasB_values[i] = NAN;
    gasC_values[i] = NAN;
  }
}


void handleSensorMeasurement(Adafruit_BME680 &bme, int csPin, MeasureState &state, unsigned long &measureStart, int profileTemp, int tiempoCiclo, const int tiemposMedida[], int num, float gasValues[], char sensorLabel) {
  digitalWrite(csPin, LOW);
  if (state == IDLE) {
    bme.setGasHeater(profileTemp, 100);
    if (bme.beginReading()) {
      state = MEASURING;
      measureStart = millis();
    }
  } else if (state == MEASURING) {
    if (bme.endReading()) {
      int i = getIndiceMedida(tiempoCiclo, tiemposMedida, num);
      if (i >= 0) gasValues[i] = bme.gas_resistance / 1000.0;
      state = IDLE;
    } else if (millis() - measureStart > measureTimeout) {
      state = IDLE;
    }
  }
  digitalWrite(csPin, HIGH);
}


void setup() {
  Serial.begin(115200);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, -1);
  pinMode(CS_A, OUTPUT); digitalWrite(CS_A, HIGH);
  pinMode(CS_B, OUTPUT); digitalWrite(CS_B, HIGH);
  pinMode(CS_C, OUTPUT); digitalWrite(CS_C, HIGH);
  if (!bme_A.begin()) Serial.println("Error sensor A"); else configurarBME(bme_A);
  if (!bme_B.begin()) Serial.println("Error sensor B"); else configurarBME(bme_B);
  if (!bme_C.begin()) Serial.println("Error sensor C"); else configurarBME(bme_C);
  ledcSetup(FAN_CHANNEL, 25000, 8);
  ledcAttachPin(FAN_PIN, FAN_CHANNEL);
  ledcWrite(FAN_CHANNEL, 0);
}


void loop() {
  static unsigned long startMillis = 0;
  static int pasoConfig = 0;
  static int ultimoSegundoEnviado = -1; // NUEVO: Para evitar duplicados


  if (!corriendo && pasoConfig == 0) {
    if (millis() - lastReadyMillis >= readyInterval) {
      lastReadyMillis = millis();
      Serial.println("READY");
    }
  }


  if (!corriendo) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      switch (pasoConfig) {
        case 0: ensayoNombre = input; Serial.println("CONF"); pasoConfig++; break;
        case 1: tipoMuestra = input; pasoConfig++; break;
        case 2: camaraEstado = input; pasoConfig++; break;
        case 3: ensayoDuracion = input.toInt(); pasoConfig++; break;
        case 4: fanPorcentaje = constrain(input.toInt(), 0, 100); pasoConfig++; break;
        case 5:
          if (input.equalsIgnoreCase("ENTER")) {
            corriendo = true;
            startMillis = millis();
            startMillis_A = startMillis; startMillis_B = startMillis; startMillis_C = startMillis;
            int pwm = map(fanPorcentaje, 0, 100, 0, 255);
            ledcWrite(FAN_CHANNEL, pwm);
            resetGasArrays();
            state_A = IDLE;
            state_B = IDLE;
            state_C = IDLE;

            measureStart_A = 0;
            measureStart_B = 0;
            measureStart_C = 0;
            ultimoSegundoEnviado = -1; // Reset del control de tiempo
            Serial.println("INICIO");
          }
          break;
      }
    }
    return;
  }


  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorMillis < sensorInterval) return;
  lastSensorMillis = currentMillis;


  int tA = (currentMillis - startMillis_A) / 1000;
  int tB = (currentMillis - startMillis_B) / 1000;
  int tC = (currentMillis - startMillis_C) / 1000;


  int duracionTotalA = 25; // Simplificado según tus arrays HP_A_MEASURE_TIMES
  int duracionTotalB = 25;
  int duracionTotalC = 25;


  int tiempoCicloA = tA % 26; // Ciclos de 26 segundos según tus perfiles
  int tiempoCicloB = tB % 26;
  int tiempoCicloC = tC % 26;


  // Manejo de mediciones (No bloqueante)
  handleSensorMeasurement(bme_A, CS_A, state_A, measureStart_A, getProfileTemperature(HP_A, HP_A_STEPS, tA), tiempoCicloA, HP_A_MEASURE_TIMES, 10, gasA_values, 'A');
  handleSensorMeasurement(bme_B, CS_B, state_B, measureStart_B, getProfileTemperature(HP_B, HP_B_STEPS, tB), tiempoCicloB, HP_B_MEASURE_TIMES, 10, gasB_values, 'B');
  handleSensorMeasurement(bme_C, CS_C, state_C, measureStart_C, getProfileTemperature(HP_C, HP_C_STEPS, tC), tiempoCicloC, HP_C_MEASURE_TIMES, 10, gasC_values, 'C');


  // --- LÓGICA DE ENVÍO CORREGIDA ---
  int segundoActual = (currentMillis - startMillis) / 1000;


  if (segundoActual != ultimoSegundoEnviado) {
    // Solo enviamos si hay al menos una medición nueva en cualquier sensor
    if (getIndiceMedida(tiempoCicloA, HP_A_MEASURE_TIMES, 10) >= 0 ||
        getIndiceMedida(tiempoCicloB, HP_B_MEASURE_TIMES, 10) >= 0 ||
        getIndiceMedida(tiempoCicloC, HP_C_MEASURE_TIMES, 10) >= 0) {
     
      String dataLine = "DATA|" + String(segundoActual) + "|";
      // NUEVO ORDEN AGRUPADO: Temp_A, Temp_B, Temp_C, Hum_A, Hum_B, Hum_C, Pres_A, Pres_B, Pres_C
      dataLine += String(bme_A.temperature, 2) + "|" + String(bme_B.temperature, 2) + "|" + String(bme_C.temperature, 2) + "|";  // Temperaturas
      dataLine += String(bme_A.humidity, 2) + "|" + String(bme_B.humidity, 2) + "|" + String(bme_C.humidity, 2) + "|";      // Humedades
      dataLine += String(bme_A.pressure / 100.0, 2) + "|" + String(bme_B.pressure / 100.0, 2) + "|" + String(bme_C.pressure / 100.0, 2) + "|";  // Presiones
     
      for (int i = 0; i < 10; i++) dataLine += (isnan(gasA_values[i]) ? "NAN" : String(gasA_values[i], 2)) + "|";
      for (int i = 0; i < 10; i++) dataLine += (isnan(gasB_values[i]) ? "NAN" : String(gasB_values[i], 2)) + "|";
      for (int i = 0; i < 9; i++)  dataLine += (isnan(gasC_values[i]) ? "NAN" : String(gasC_values[i], 2)) + "|";
      dataLine += (isnan(gasC_values[9]) ? "NAN" : String(gasC_values[9], 2)) + "|" + String(fanPorcentaje);
     
      Serial.println(dataLine);
      ultimoSegundoEnviado = segundoActual; // Bloquea hasta que el reloj cambie de segundo


      // Limpiar valores enviados
      for (int i = 0; i < 10; i++) {
        gasA_values[i] = NAN; gasB_values[i] = NAN; gasC_values[i] = NAN;
      }
    }
  }


  // Fin por tiempo
  if (segundoActual >= ensayoDuracion) {
    Serial.println("FIN");
    corriendo = false; pasoConfig = 0;
    resetGasArrays();
    state_A = IDLE;
    state_B = IDLE;
    state_C = IDLE;

    measureStart_A = 0;
    measureStart_B = 0;
    measureStart_C = 0;
    ledcWrite(FAN_CHANNEL, 0);
  }


  // STOP manual
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n'); cmd.trim();
    if (cmd.equalsIgnoreCase("STOP")) {
      resetGasArrays();

      state_A = IDLE;
      state_B = IDLE;
      state_C = IDLE;

      measureStart_A = 0;
      measureStart_B = 0;
      measureStart_C = 0;
      Serial.println("STOPPED");
      corriendo = false; pasoConfig = 0;
      resetGasArrays();
      ledcWrite(FAN_CHANNEL, 0);
    }
  }
}

