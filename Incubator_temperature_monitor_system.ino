#include <Wire.h>
#include <LiquidCrystal.h>
#include <math.h>

// Pinos 
const int PIN_NTC    = A0;  // Sensor NTC
const int PIN_POT    = A1;  // Potenciômetro set point
const int PIN_PWM    = 3;   // Saída PWM carga resistiva
const int PIN_LED    = 2;   // LED alarme ≥38°C

// Parâmetros NTC
const float R_SERIE   = 10000.0; // 10kΩ
const float R_NOM     = 10000.0; // 10kΩ @ 25°C
const float T_NOM     = 298.15;  // 25°C em Kelvin
const float BETA      = 3950.0;  // Coef. β NTC MF58
const float V_REF     = 5.0;     // Tensão referência ADC
const float ADC_MAX   = 1023.0;

// Faixa Set Point (pot) 
const float TEMP_MIN  = 20.0;  // °C mínimo
const float TEMP_MAX  = 38.0;  // °C máximo
const float TEMP_ALRM = 38.0;  // °C → LED alarme

// Controle Proporcional 
const float Kp        = 25.5;  // Ganho proporcional
const float BANDA_P   = 10.0;  // Banda proporcional (°C)

// LCD 
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Variáveis globais 
float tempMedida   = 0.0;
float tempSetPoint = 37.0;
int   pwmSaida     = 0;
unsigned long tUltLCD = 0;

// ============================================================
float lerTemperaturaNTC() {
  int adc = analogRead(PIN_NTC);
  float v = (adc / ADC_MAX) * V_REF;
  float rNTC = R_SERIE * v / (V_REF - v);
  float tempK = 1.0 / ((1.0 / T_NOM) + log(rNTC / R_NOM) / BETA);
  return tempK - 273.15;
}

float lerSetPoint() {
  int adc = analogRead(PIN_POT);
  return TEMP_MIN + (adc / ADC_MAX) * (TEMP_MAX - TEMP_MIN);
}

int calcularPWM(float erro) {
  // Controle proporcional: PWM = Kp * erro, limitado a [0, 255]
  float saida = Kp * erro;
  saida = constrain(saida, 0, 255);
  return abs(255 - (int)saida);
}

void atualizarLCD() {
  lcd.setCursor(0, 0);
  lcd.print("Alvo:");
  lcd.print(tempSetPoint, 1);
  lcd.print("\337C   ");  // \337 = símbolo °

  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(tempMedida, 1);
  lcd.print("\337C   ");
}

// ============================================================
void setup() {
  pinMode(PIN_PWM, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  analogWrite(PIN_PWM, 0);
  digitalWrite(PIN_LED, LOW);

  lcd.begin(16, 2);
  lcd.setCursor(2, 1); lcd.print("Iniciando...");
  delay(2000);
  lcd.clear();

  Serial.begin(9600);
  Serial.println("=== Aquecimento iniciado ===");
}

void loop() {
  // 1. Leituras
  tempSetPoint = lerSetPoint();
  tempMedida   = lerTemperaturaNTC();
  float erro   = tempSetPoint - tempMedida;

  // 2. Controle proporcional
  if (tempMedida >= TEMP_ALRM) {
    // Alarme: desliga aquecimento imediatamente
    pwmSaida = 0;
    digitalWrite(PIN_LED, HIGH);
  } else {
    digitalWrite(PIN_LED, LOW);
    pwmSaida = calcularPWM(erro);
  }

  analogWrite(PIN_PWM, pwmSaida);

  // 3. Atualiza LCD a cada 500ms
  if (millis() - tUltLCD >= 500) {
    atualizarLCD();
    tUltLCD = millis();
  }

  // 4. Serial monitor (debug)
  Serial.print("SP:"); Serial.print(tempSetPoint, 1);
  Serial.print("  T:"); Serial.print(tempMedida, 1);
  Serial.print("  Err:"); Serial.print(erro, 2);
  Serial.print("  PWM:"); Serial.println(pwmSaida);

  delay(100); // Loop a 10Hz
}