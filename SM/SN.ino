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

// === Variables globales ===
String ensayoNombre = "";
String tipoMuestra = "";
String camaraEstado = "";
unsigned int ensayoDuracion = 0; // segundos
int fanPorcentaje = 0;
bool corriendo = false;

// --- Temporizadores ---
unsigned long lastSensorMillis = 0;
unsigned long lastGasMillis = 0;
const unsigned long sensorInterval = 500;   // ms para T/H/P
const unsigned long gasInterval = 2000;     // ms para gas

// Variables de gas guardadas
float gasA = NAN, gasB = NAN, gasC = NAN;

// Configuración común de los sensores
void configurarBME(Adafruit_BME680 &bme) {
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

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

  Serial.println("READY");
}

void loop() {
  static unsigned long startMillis = 0;
  static int pasoConfig = 0;

  if (!corriendo) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();

      switch (pasoConfig) {
        case 0:
          ensayoNombre = input;
          Serial.println("CONF");
          pasoConfig++;
          break;
        case 1:
          tipoMuestra = input;
          pasoConfig++;
          break;
        case 2:
          camaraEstado = input;
          pasoConfig++;
          break;
        case 3:
          ensayoDuracion = input.toInt();
          pasoConfig++;
          break;
        case 4:
          fanPorcentaje = constrain(input.toInt(), 0, 100);
          pasoConfig++;
          break;
        case 5:
          if (input.equalsIgnoreCase("ENTER")) {
            corriendo = true;
            startMillis = millis();
            int fanPWM = map(fanPorcentaje, 0, 100, 0, 255);
            ledcWrite(FAN_CHANNEL, fanPWM);
            Serial.println("INICIO");
          }
          break;
      }
    }
    return;
  }

  // --- Ensayo corriendo ---
  if (corriendo) {
    // Comprobar STOP
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();
      if (cmd.equalsIgnoreCase("STOP")) {
        corriendo = false;
        ledcWrite(FAN_CHANNEL, 0);
        Serial.println("STOPPED");
        pasoConfig = 0; // Preparar para siguiente fase
        return;
      }
    }

    unsigned long t = (millis() - startMillis) / 1000;
    if (t >= ensayoDuracion) {
      corriendo = false;
      ledcWrite(FAN_CHANNEL, 0);
      Serial.println("FIN");
      pasoConfig = 0; // Preparar para siguiente fase
      return;
    }

    unsigned long currentMillis = millis();

    // --- Leer T/H/P cada 0.5 s ---
    if (currentMillis - lastSensorMillis >= sensorInterval) {
      lastSensorMillis = currentMillis;

      float tempA = NAN, tempB = NAN, tempC = NAN;
      float humA = NAN, humB = NAN, humC = NAN;
      float presA = NAN, presB = NAN, presC = NAN;

      // Sensor A
      digitalWrite(CS_A, LOW); digitalWrite(CS_B, HIGH); digitalWrite(CS_C, HIGH);
      if (bme_A.performReading()) {
        tempA = bme_A.temperature; humA = bme_A.humidity; presA = bme_A.pressure / 100.0;
      }
      digitalWrite(CS_A, HIGH);

      // Sensor B
      digitalWrite(CS_A, HIGH); digitalWrite(CS_B, LOW); digitalWrite(CS_C, HIGH);
      if (bme_B.performReading()) {
        tempB = bme_B.temperature; humB = bme_B.humidity; presB = bme_B.pressure / 100.0;
      }
      digitalWrite(CS_B, HIGH);

      // Sensor C
      digitalWrite(CS_A, HIGH); digitalWrite(CS_B, HIGH); digitalWrite(CS_C, LOW);
      if (bme_C.performReading()) {
        tempC = bme_C.temperature; humC = bme_C.humidity; presC = bme_C.pressure / 100.0;
      }
      digitalWrite(CS_C, HIGH);

      // --- Leer Gas cada 2 s ---
      if (currentMillis - lastGasMillis >= gasInterval) {
        lastGasMillis = currentMillis;

        digitalWrite(CS_A, LOW); digitalWrite(CS_B, HIGH); digitalWrite(CS_C, HIGH);
        if (bme_A.performReading()) gasA = bme_A.gas_resistance / 1000.0;
        digitalWrite(CS_A, HIGH);

        digitalWrite(CS_A, HIGH); digitalWrite(CS_B, LOW); digitalWrite(CS_C, HIGH);
        if (bme_B.performReading()) gasB = bme_B.gas_resistance / 1000.0;
        digitalWrite(CS_B, HIGH);

        digitalWrite(CS_A, HIGH); digitalWrite(CS_B, HIGH); digitalWrite(CS_C, LOW);
        if (bme_C.performReading()) gasC = bme_C.gas_resistance / 1000.0;
        digitalWrite(CS_C, HIGH);
      }

      // --- Enviar paquete CSV completo ---
      Serial.print("DATA|"); Serial.print(t); Serial.print("|");
      Serial.print(tempA); Serial.print("|"); Serial.print(tempB); Serial.print("|"); Serial.print(tempC); Serial.print("|");
      Serial.print(humA); Serial.print("|"); Serial.print(humB); Serial.print("|"); Serial.print(humC); Serial.print("|");
      Serial.print(presA); Serial.print("|"); Serial.print(presB); Serial.print("|"); Serial.print(presC); Serial.print("|");
      Serial.print(gasA); Serial.print("|"); Serial.print(gasB); Serial.print("|"); Serial.print(gasC); Serial.print("|");
      Serial.println(map(fanPorcentaje, 0, 100, 0, 255));
    }
  }
}
