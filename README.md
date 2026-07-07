
# RELATÓRIO TÉCNICO: CONTROLE PWM DE ATUADORES E MOTORES COM ESP32 NO SIMULADOR WOKWI

## 1. RESUMO

Este trabalho apresenta o desenvolvimento, a simulação e a validação de sistemas 
microcontrolados baseados em Modulação por Largura de Pulso (PWM) utilizando a 
plataforma ESP32 DevKit dentro do ambiente virtual Wokwi. O projeto é dividido 
em duas vertentes complementares de engenharia embarcada. A primeira aborda o 
controle posicional analógico de um servomotor utilizando a biblioteca padrão 
"ESP32Servo", cuja variação de Duty Cycle (0 a 100%) ocorre via manipulação 
manual de um transdutor resistivo (potenciômetro). A segunda consiste em uma 
aplicação industrial avançada que emprega a biblioteca nativa do chip, a MCPWM 
(Motor Control PWM), para regular a potência e velocidade de um motor DC. 
O sistema final agrega tratamento de interrupções externas para inversão do 
sentido de rotação por botão físico, interface visual síncrona via barramento 
I2C com display OLED SSD1306 e monitoramento dinâmico em tempo real por meio 
de telemetria serial.


## 2. INTRODUÇÃO

A Modulação por Largura de Pulso (PWM - Pulse Width Modulation) constitui uma das 
técnicas mais eficientes e difundidas na engenharia de sistemas embarcados para o 
controle de potência entregue a cargas elétricas lineares e não lineares, tais 
como elementos aquecedores, LEDs e atuadores eletromecânicos. Ao variar a fração 
de tempo em que o sinal permanece em nível lógico alto dentro de um período fixo 
(Duty Cycle), altera-se a tensão média equivalente aplicada ao dispositivo.

No ecossistema do SoC (System-on-Chip) ESP32, as capacidades de geração de sinais 
PWM expandem-se imensamente se comparadas a microcontroladores de 8 bits tradicionais. 
O circuito integrado dispõe de periféricos dedicados de altíssimo desempenho. 
Entre eles, destacam-se o módulo LEDC (focado em iluminação e controle básico de 
servos) e o avançado hardware MCPWM (Motor Control PWM). O MCPWM foi projetado 
estruturalmente para controle complexo de motores trifásicos, pontes H, inversores 
de frequência e robótica de alta precisão. 

Este relatório detalha os fundamentos teóricos e práticos de programação desses 
periféricos por meio de dois exercícios práticos de laboratório, demonstrando como 
converter variáveis analógicas externas coletadas por conversores analógico-digitais 
(ADC) em sinais dinâmicos de atuação mecânica, telemetria serial e exibição 
visual estável em telas OLED.


## 3. ARQUIVO DE CONFIGURAÇÃO DO SIMULADOR (diagram.json)

Tambem pode ser encontrado fotos nos arquivos separados anexados

Para reproduzir o circuito completo no simulador virtual Wokwi, o seguinte código 
deve ser colado integralmente na aba "diagram.json":

{
  "version": 1,
  "author": "Anonymous Maker",
  "editor": "wokwi",
  "parts": [
    { "type": "board-esp32-devkit-c-v4", "id": "esp32", "top": 0, "left": 0, "attrs": {} },
    { "type": "wokwi-rgb-led", "id": "rgb1", "top": 120, "left": -200, "attrs": { "common": "cathode" } },
    { "type": "wokwi-resistor", "id": "r1", "top": 130, "left": -100, "attrs": { "value": "220" } },
    { "type": "wokwi-resistor", "id": "r2", "top": 160, "left": -100, "attrs": { "value": "220" } },
    { "type": "wokwi-resistor", "id": "r3", "top": 190, "left": -100, "attrs": { "value": "220" } },
    { "type": "wokwi-servo", "id": "servo1", "top": 100, "left": 200, "attrs": {} },
    { "type": "wokwi-potentiometer", "id": "pot1", "top": -120, "left": -120, "attrs": {} },
    { "type": "board-ssd1306", "id": "oled1", "top": -150, "left": 100, "attrs": {} },
    { "type": "wokwi-pushbutton", "id": "btn1", "top": 250, "left": 50, "attrs": { "color": "red" } },
    { "type": "wokwi-motor-dc", "id": "motor1", "top": 220, "left": 250, "attrs": {} }
  ],
  "connections": [
    [ "esp32:GND.1", "rgb1:COM", "black", [ "v0" ] ],
    [ "esp32:25", "r1:1", "green", [ "v0" ] ],
    [ "esp32:26", "r2:1", "green", [ "v0" ] ],
    [ "esp32:27", "r3:1", "green", [ "v0" ] ],
    [ "r1:2", "rgb1:R", "red", [ "v0" ] ],
    [ "r2:2", "rgb1:G", "green", [ "v0" ] ],
    [ "r3:2", "rgb1:B", "blue", [ "v0" ] ],
    
    [ "esp32:GND.1", "servo1:GND", "black", [ "v0" ] ],
    [ "esp32:5V", "servo1:V+", "red", [ "v0" ] ],
    [ "esp32:18", "servo1:PWM", "green", [ "v0" ] ],
    
    [ "esp32:GND.2", "pot1:GND", "black", [ "v0" ] ],
    [ "esp32:3V3", "pot1:VCC", "red", [ "v0" ] ],
    [ "esp32:34", "pot1:SIG", "green", [ "v0" ] ],
    
    [ "esp32:GND.2", "oled1:GND", "black", [ "v0" ] ],
    [ "esp32:3V3", "oled1:VCC", "red", [ "v0" ] ],
    [ "esp32:22", "oled1:SCL", "yellow", [ "v0" ] ],
    [ "esp32:21", "oled1:SDA", "blue", [ "v0" ] ],
    
    [ "esp32:GND.1", "btn1:2.G", "black", [ "v0" ] ],
    [ "esp32:14", "btn1:1.R", "orange", [ "v0" ] ],
    
    [ "esp32:GND.1", "motor1:GND", "black", [ "v0" ] ],
    [ "esp32:19", "motor1:VCC", "purple", [ "v0" ] ]
  ],
  "dependencies": {
    "ESP32Servo": "",
    "Adafruit SSD1306": "",
    "Adafruit GFX Library": ""
  }
}


## 4. EXERCÍCIO 1: CONTROLE DO SERVOMOTOR (TEORIA E CÓDIGO)

4.1 Explicação Detalhada do Exercício 1
No primeiro exercício, o objetivo central reside em traduzir o deslocamento 
mecânico de um potenciômetro na mudança angular exata do braço de um servomotor. 
O sinal analógico de tensão enviado pelo cursor do potenciômetro (0 a 3.3V) adentra 
o canal ADC do ESP32 (GPIO 34), que possui uma resolução nativa de 12 bits, gerando 
uma escala numérica digitalizada que varia estritamente de 0 a 4095.

A biblioteca "ESP32Servo" simplifica radicalmente esse controle ao abstrair os 
cálculos de hardware necessários para gerar a onda quadrada padrão demandada por 
servomotores (frequência fixa em 50 Hz, onde um pulso em nível alto de 1 ms define 
a posição de 0 graus e um pulso de 2 ms define a posição de 180 graus). No loop do 
programa, a função map() recebe a variável crua do ADC e realiza uma interpolação 
linear para converter o valor digital para um ângulo físico de 0 a 180 graus. O Duty 
Cycle equivalente varia de 0 a 100% proporcionalmente à leitura, sendo enviado 
diretamente ao pino através do método .write().

4.2 Código Comentado - Exercício 1 Isolado
Abaixo encontra-se a implementação exclusiva do controle do Servo por Potenciômetro:

// =============================================================================
// CÓDIGO EXERCÍCIO 1: CONTROLE DE SERVO VIA POTENCIÔMETRO (LEDC/ESP32SERVO)
// =============================================================================
#include <Arduino.h>
#include <ESP32Servo.h>

const int POT_PIN   = 34;  // Pino de entrada analógica ligado ao potenciômetro
const int SERVO_PIN = 18;  // Pino de saída PWM ligado ao sinal do Servomotor

Servo meuServo;            // Criação do objeto para controle do servomotor

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("--- Inicializando Exercício 1: Servo Motor ---");
  
  meuServo.attach(SERVO_PIN); // Vincula o pino do servo ao objeto de controle
}

void loop() {
  // Leitura do canal analógico (Resolução de 12 bits: 0 a 4095)
  int valorADC = analogRead(POT_PIN);
  
  // Mapeamento linear do ADC (0-4095) para a faixa de ângulo do servo (0-180 graus)
  int anguloServo = map(valorADC, 0, 4095, 0, 180);
  
  // Cálculo teórico do Duty Cycle correspondente em porcentagem (0 a 100%)
  float dutyCycleServo = (valorADC / 4095.0) * 100.0;
  
  // Atualiza a posição do servomotor com o ângulo calculado
  meuServo.write(anguloServo);
  
  // Exibição dos dados coletados e processados no Monitor Serial
  Serial.printf("ADC Potenciometro: %d | Angulo do Servo: %d deg | Duty Servo: %.1f%%
", 
                valorADC, anguloServo, dutyCycleServo);
                
  delay(200); // Janela de amostragem estável de 200 milissegundos
}

## 5. EXERCÍCIO 2: CONTROLE AVANÇADO DE MOTOR DC VIA MCPWM (TEORIA E CÓDIGO)

5.1 Explicação Detalhada do Exercício 2
O segundo exercício expande a aplicação para o ambiente de acionamento industrial 
ao explorar a periferia nativa MCPWM (Motor Control PWM) do ESP32, estruturada 
de forma independente da API do Arduino para operar diretamente nos registradores 
de hardware do chip através da inclusão do cabeçalho "driver/mcpwm.h". Essa abordagem 
independente mitiga o uso do processador principal para atualizar os temporizadores 
do PWM, erradicando oscilações indesejadas (jitter) sob altas cargas de trabalho.

A velocidade do motor DC é calculada normalizando a leitura digital obtida pelo 
potenciômetro em uma escala percentual em ponto flutuante de 0.0% a 100.0%. A função 
nativa mcpwm_set_duty() injeta esse percentual em tempo real no comparador do timer.
Para agregar valor técnico e cumprir os requisitos de hardware adicional, o sistema 
incorpora um botão de interrupção externa (GPIO 14) configurado com pull-up interno. 
Sempre que pressionado, ele gera um gatilho de borda de descida (FALLING) gerenciado 
pela macro IRAM_ATTR, invertendo de forma instantânea uma variável de flag que monitora 
o sentido da rotação (Horário / Anti-horário), utilizando filtragem por software 
(debounce) para suprimir ruídos de contatos elétricos. Todas as variáveis operacionais 
são transmitidas e renderizadas graficamente em um display OLED SSD1306 via I2C.


## 6. CÓDIGO UNIFICADO INTEGRADO (SISTEMA COMPLETO)

Este é o código unificado final rodando no simulador Wokwi. Ele integra de maneira 
assíncrona e otimizada o controle de LEDs RGB (via LEDC atualizado do ESP32 v3.0+), 
o Servomotor (Exercício 1), o Motor DC via MCPWM nativo (Exercício 2), tratamento 
de interrupção e atualização do Display OLED por barramento I2C.

Encontra-se no arquivo codigo.c


## 7. CONCLUSÃO

Os testes práticos e ensaios conduzidos no ambiente virtual de simulação do Wokwi 
comprovaram com sucesso o funcionamento simultâneo, concorrente e harmônico de ambos 
os sistemas de controle de potência baseados em PWM. A biblioteca "ESP32Servo" 
mostrou-se uma excelente ferramenta de rápida prototipagem e altíssima estabilidade 
para tarefas gerais de posicionamento angular mecânico.

Paralelamente, a implementação avançada utilizando o periférico de baixo nível 
MCPWM confirmou a superioridade arquitetural do ESP32 frente a hardwares comuns, 
oferecendo precisão estrita no gerenciamento do Duty Cycle sobre motores DC e drivers 
sem gerar sobrecarga de processamento nas CPUs principais (núcleos Xtensa). A correta 
integração dos periféricos adicionais exigidos — comunicação serial assíncrona, 
interrupções externas tratadas contra ruídos elétricos e exibição síncrona via 
barramento de comunicação I2C com display OLED — consolidou um protótipo integrado 
robusto, robustecido por boas práticas de programação aplicada a sistemas embarcados.
