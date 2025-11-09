// Sketch para ESP32: Adquisición indefinida de 3 sensores BME680 vía SPI
// Usa CS pins para seleccionar sensores.
// Formato Serial: DATA|t|TempA|TempB|TempC|HumA|HumB|HumC|PresA|PresB|PresC|GasA|GasB|GasC|PWM
// Temporizador: millis() en segundos desde inicio.
// LED GPIO 15 parpadea (500ms ciclo) durante adquisición indefinida.
// Lecturas cada 2000 ms (gas estable; T/H/P leídos siempre).
// PWM ventilador: 0 fijo (GPIO 2 configurado pero off).
// Librerías: SPI.h, Adafruit_Sensor.h, Adafruit_BME680.h
// Pines: SPI (SCK=12, MOSI=11, MISO=10), CS (8=A,7=B,6=C), FAN=2, LED=15
// Compilado: __DATE__ __TIME__

#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

// Pines SPI y CS (exactos del referencia)
#define SCK_PIN 12
#define MOSI_PIN 11
#define MISO_PIN 10
#define CS_A 8
#define CS_B 7
#define CS_C 6

// Ventilador (configurado pero fijo 0)
#define FAN_PIN 2
#define FAN_CHANNEL 0

// LED parpadeante
#define LED_PIN 15

// Sensores BME680 SPI
Adafruit_BME680 bme_A(CS_A);
Adafruit_BME680 bme_B(CS_B);
Adafruit_BME680 bme_C(CS_C);

// Variables temporizador y parpadeo
unsigned long tiempo_inicio = 0;
unsigned long ultima_lectura = 0;
unsigned long ultimo_parpadeo = 0;
bool led_estado = false;
const unsigned long intervalo_lectura = 2000;  // 2 segundos (para gas estable, como referencia)
const unsigned long intervalo_parpadeo = 250;  // 250ms on/off para 500ms ciclo

// Variables datos (agrupadas por tipo, como referencia)
float tempA = NAN, tempB = NAN, tempC = NAN;
float humA = NAN, humB = NAN, humC = NAN;
float presA = NAN, presB = NAN, presC = NAN;
float gasA = NAN, gasB = NAN, gasC = NAN;
int pwm = 0;  // Fijo en 0

// Flags para inicialización
bool bmeA_ok = false, bmeB_ok = false, bmeC_ok = false;

void configurarBME(Adafruit_BME680 &bme) {
  // Configuración exacta del referencia
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);  // Heater para gas
}

void setup() {
  Serial.begin(115200);
  while (!Serial);  // Espera serial estable
  delay(1000);

  // Inicializar SPI con pines del referencia
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, -1);

  // Configurar CS pins como salida y HIGH (desactivado)
  pinMode(CS_A, OUTPUT); digitalWrite(CS_A, HIGH);
  pinMode(CS_B, OUTPUT); digitalWrite(CS_B, HIGH);
  pinMode(CS_C, OUTPUT); digitalWrite(CS_C, HIGH);

  // Inicializar sensores (como referencia)
  if (!bme_A.begin()) {
    Serial.println("❌ Error inicializando sensor A");
  } else {
    configurarBME(bme_A);
    bmeA_ok = true;
    Serial.println("✅ BME A inicializado correctamente.");
  }

  if (!bme_B.begin()) {
    Serial.println("❌ Error inicializando sensor B");
  } else {
    configurarBME(bme_B);
    bmeB_ok = true;
    Serial.println("✅ BME B inicializado correctamente.");
  }

  if (!bme_C.begin()) {
    Serial.println("❌ Error inicializando sensor C");
  } else {
    configurarBME(bme_C);
    bmeC_ok = true;
    Serial.println("✅ BME C inicializado correctamente.");
  }

  // Configurar ventilador PWM
  ledcSetup(FAN_CHANNEL, 25000, 8);
  ledcAttachPin(FAN_PIN, FAN_CHANNEL);
  ledcWrite(FAN_CHANNEL, 0);  // Off

  // Configurar LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Iniciar temporizador
  tiempo_inicio = millis();

  Serial.println("Iniciando adquisición indefinida de sensores BME680 vía SPI...");
  Serial.println("Formato: DATA|t|TempA|TempB|TempC|HumA|HumB|HumC|PresA|PresB|PresC|GasA|GasB|GasC|PWM");
  Serial.println("LED GPIO 15 parpadeando (adquisición en curso).");
  Serial.println("Tiempo (t) en segundos desde inicio. Lecturas cada 2s...\n");

  // Calibración inicial para heaters de gas (como referencia, ~2s)
  Serial.println("Calibrando heaters de gas (espera 2s)...");
  delay(2000);

  Serial.println("Adquisición iniciada.\n");
  ultima_lectura = millis();
}

void loop() {
  unsigned long ahora = millis();

  // Parpadeo LED (no-bloqueante, siempre activo)
  if (ahora - ultimo_parpadeo >= intervalo_parpadeo) {
    led_estado = !led_estado;
    digitalWrite(LED_PIN, led_estado ? HIGH : LOW);
    ultimo_parpadeo = ahora;
  }

  // Lectura indefinida cada intervalo_lectura (2000 ms)
  if (ahora - ultima_lectura >= intervalo_lectura) {
    unsigned long t = (ahora - tiempo_inicio) / 1000;  // Tiempo en segundos

    // Resetear variables a NaN por seguridad
    tempA = tempB = tempC = NAN;
    humA = humB = humC = NAN;
    presA = presB = presC = NAN;
    gasA = gasB = gasC = NAN;

    // Leer Sensor A (manejo CS como referencia)
    if (bmeA_ok) {
      digitalWrite(CS_A, LOW); digitalWrite(CS_B, HIGH); digitalWrite(CS_C, HIGH);
      if (bme_A.performReading()) {
        tempA = bme_A.temperature;
        humA = bme_A.humidity;
        presA = bme_A.pressure / 100.0F;  // hPa
        gasA = bme_A.gas_resistance / 1000.0F;  // kΩ
      }
      digitalWrite(CS_A, HIGH);
    }

    // Leer Sensor B
    if (bmeB_ok) {
      digitalWrite(CS_A, HIGH); digitalWrite(CS_B, LOW); digitalWrite(CS_C, HIGH);
      if (bme_B.performReading()) {
        tempB = bme_B.temperature;
        humB = bme_B.humidity;
        presB = bme_B.pressure / 100.0F;
        gasB = bme_B.gas_resistance / 1000.0F;
      }
      digitalWrite(CS_B, HIGH);
    }

    // Leer Sensor C
    if (bmeC_ok) {
      digitalWrite(CS_A, HIGH); digitalWrite(CS_B, HIGH); digitalWrite(CS_C, LOW);
      if (bme_C.performReading()) {
        tempC = bme_C.temperature;
        humC = bme_C.humidity;
        presC = bme_C.pressure / 100.0F;
        gasC = bme_C.gas_resistance / 1000.0F;
      }
      digitalWrite(CS_C, HIGH);
    }

    // Imprimir paquete CSV (formato del referencia)
    Serial.print("DATA|");
    Serial.print(t);
    Serial.print("|");
    Serial.print(tempA, 1); Serial.print("|"); Serial.print(tempB, 1); Serial.print("|"); Serial.print(tempC, 1);
    Serial.print("|");
    Serial.print(humA, 1); Serial.print("|"); Serial.print(humB, 1); Serial.print("|"); Serial.print(humC, 1);
    Serial.print("|");
    Serial.print(presA, 2); Serial.print("|"); Serial.print(presB, 2); Serial.print("|"); Serial.print(presC, 2);
    Serial.print("|");
    Serial.print(gasA, 1); Serial.print("|"); Serial.print(gasB, 1); Serial.print("|"); Serial.print(gasC, 1);
    Serial.print("|");
    Serial.print(pwm);  // 0 fijo
    Serial.println();

    ultima_lectura = ahora;
  }
}