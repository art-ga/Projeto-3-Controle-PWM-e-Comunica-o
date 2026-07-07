#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "driver/mcpwm.h" // Biblioteca nativa do ESP32 para controle de motores

// Definições de Pinos
const int POT_PIN   = 34;  // Entrada analógica do Potenciômetro
const int SERVO_PIN = 18;  // Saída PWM para o Servo Motor
const int RED_PIN   = 25;  // LED RGB - Vermelho
const int GREEN_PIN = 26;  // LED RGB - Verde
const int BLUE_PIN  = 27;  // LED RGB - Azul
const int BUTTON_PIN = 14; // Botão de Inversão (Interrupção)
const int MOTOR_PIN  = 19; // Saída MCPWM para o Motor DC

// Configurações do Display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Objetos e Variáveis de Controle
Servo meuServo;
volatile bool sentidoHorario = true; // Modificada na Interrupção
unsigned long ultimoDebounce = 0;
const unsigned long TEMPO_DEBOUNCE_MS = 200;

// Variáveis de Temporização para o Loop
unsigned long ultimaAtualizacao = 0;
const unsigned long INTERVALO_MS = 200;

// Variáveis do LED RGB (Mantendo a lógica anterior)
const int PWM_FREQ = 5000;
const int PWM_RES  = 8;
int redDuty = 0, greenDuty = 0, blueDuty = 0;

// Função de Interrupção para o Botão (Debounce por Software)
void IRAM_ATTR tratarBotao() {
  if ((millis() - ultimoDebounce) > TEMPO_DEBOUNCE_MS) {
    sentidoHorario = !sentidoHorario;
    ultimoDebounce = millis();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== SISTEMA INTEGRADO: SERVO (ESP32Servo) & MOTOR (MCPWM) ===");

  // 1. Inicialização do Display OLED I2C
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Falha ao inicializar o display OLED!");
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // 2. Configuração do LED RGB (LEDC v3.0+)
  ledcAttach(RED_PIN,   PWM_FREQ, PWM_RES);
  ledcAttach(GREEN_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(BLUE_PIN,  PWM_FREQ, PWM_RES);

  // 3. Configuração da PARTE 1: Servomotor (ESP32Servo)
  meuServo.attach(SERVO_PIN);

  // 4. Configuração da PARTE 2: MCPWM Nativo para o Motor DC
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, MOTOR_PIN); // Associa o pino 19 ao MCPWM Unidade 0, Sinal 0A
  
  mcpwm_config_t mcpwm_config;
  mcpwm_config.frequency = 1000;                  // Frequência de 1kHz para motores DC
  mcpwm_config.cmpr_a = 0.0;                      // Duty cycle inicial 0%
  mcpwm_config.cmpr_b = 0.0;
  mcpwm_config.counter_mode = MCPWM_UP_COUNTER;   // Modo de contagem progressiva
  mcpwm_config.duty_mode = MCPWM_DUTY_MODE_0;     // Duty cycle ativo em nível alto
  
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &mcpwm_config); // Configura os parâmetros na unidade

  // 5. Configuração do Botão com Interrupção Externa
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), tratarBotao, FALLING);
}

void loop() {
  if (millis() - ultimaAtualizacao >= INTERVALO_MS) {
    ultimaAtualizacao = millis();

    // Leitura do ADC (0 a 4095 no ESP32)
    int valorADC = analogRead(POT_PIN);

    // --- PARTE 1: Cálculo e Controle do Servo Motor ---
    // Mapeia a leitura do ADC diretamente para ângulo (0 a 180 graus)
    int anguloServo = map(valorADC, 0, 4095, 0, 180);
    // O Duty Cycle equivalente varia proporcionalmente de 0 a 100%
    float dutyCycleServo = map(valorADC, 0, 4095, 0, 100);
    meuServo.write(anguloServo);

    // --- PARTE 2: Cálculo e Controle do Motor DC via MCPWM ---
    // Converte a leitura do potenciômetro em porcentagem de velocidade (0% a 100%)
    float velocidadeMotor = (valorADC / 4095.0) * 100.0;
    
    // Aplica a velocidade calculada diretamente utilizando a API nativa do MCPWM
   mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_GEN_A, velocidadeMotor);
    // Lógica Secundária: Atualização do LED RGB conforme o ciclo
    redDuty += 5;   if (redDuty > 100) redDuty = 0;
    greenDuty += 2; if (greenDuty > 100) greenDuty = 0;
    blueDuty += 10; if (blueDuty > 100) blueDuty = 0;
    ledcWrite(RED_PIN,   map(redDuty,   0, 100, 0, 255));
    ledcWrite(GREEN_PIN, map(greenDuty, 0, 100, 0, 255));
    ledcWrite(BLUE_PIN,  map(blueDuty,  0, 100, 0, 255));

    // --- Saída de Dados: Monitor Serial ---
    Serial.println("==========================================");
    Serial.printf("ADC Potenciometro: %d\n", valorADC);
    Serial.printf("[PARTE 1] Servo Angulo: %d° | Duty: %.1f%%\n", anguloServo, dutyCycleServo);
    Serial.printf("[PARTE 2] Motor DC Vel: %.1f%% | Sentido: %s\n", 
                  velocidadeMotor, sentidoHorario ? "HORARIO" : "ANTI-HORARIO");

    // --- Saída de Dados: Display OLED I2C ---
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("     SISTEMA PWM      ");
    display.println("----------------------");
    display.printf("POT ADC : %d\n", valorADC);
    display.printf("Ang.Servo: %d deg\n", anguloServo);
    display.printf("Vel.Motor: %.1f %%\n", velocidadeMotor);
    display.printf("Sentido  : %s\n", sentidoHorario ? "HORARIO" : "ANTI-HOR");
    display.display();
  }
}
