/**
 * ===================================================================================
 * === PROJETO DE INTERFACE GRÁFICA PARA TTGO COM LAYOUTS EM MEMÓRIA INTERNA ========
 * ===================================================================================
 * Este código controla um display TFT em um ESP32 (modelo TTGO), usando um
 * layout pré-compilado a partir de um arquivo de cabeçalho (layout.h).
 * Ele gerencia a interação do usuário através de dois botões integrados
 * para ciclar entre diferentes ícones de status.
 * ===================================================================================
 */

#include <TFT_eSPI.h> // Biblioteca principal para o controle do display TFT
#include "layout.h"   // Inclui o nosso layout gerado pela ferramenta Python

// --- Pinos e Definições ---
#define BUTTON_1_PIN 0  // Botão 1 (geralmente BOOT ou IO0)
#define BUTTON_2_PIN 35 // Botão 2 (geralmente IO35)

// --- Objetos e Variáveis Globais ---
TFT_eSPI tft = TFT_eSPI(); // Objeto principal da biblioteca do display

// Variáveis para leitura dos botões
int lastButton1State = HIGH;
int lastButton2State = HIGH;

// Variáveis para o ciclo de ícones
const int numCycleIcons = 5; // O número total de ícones no seu ciclo
int currentCycleIconIndex = 0; // Começa no primeiro ícone

// =========================================================================
// === Funções de Ação para cada Botão =====================================
// =========================================================================

/**
 * @brief Avança para o próximo ícone no ciclo e chama sua função de desenho específica.
 */
void cycleNextIcon() {
  // Avança para o próximo índice na lista de ícones
  currentCycleIconIndex = (currentCycleIconIndex + 1) % numCycleIcons;

  Serial.printf("Ciclando para o icone de indice: %d\n", currentCycleIconIndex);

  // Usa um switch para chamar a função de desenho correta.
  // IMPORTANTE: Você deve substituir os nomes 'draw_bateria', 'draw_wifi', etc.,
  // pelos nomes exatos das funções geradas no seu arquivo layout.h!
  // Ex: draw_img_1_bateria_png(tft), draw_img_2_wifi_png(tft), etc.
  switch (currentCycleIconIndex) {
    case 0:
      // Exemplo: draw_bateria(tft);
      break;
    case 1:
      // Exemplo: draw_wifi(tft);
      break;
    case 2:
      // Exemplo: draw_usb(tft);
      break;
    case 3:
      // Exemplo: draw_bluetooth(tft);
      break;
    case 4:
      // Exemplo: draw_micro_sd(tft);
      break;
  }
}

// Botão 1: Quando pressionado, chama a função de ciclo de ícones.
void handleButton1() {
  Serial.println("Botao 1 Pressionado! Ciclando icone...");
  cycleNextIcon();
}

// Botão 2: Também chama a mesma função.
void handleButton2() {
  Serial.println("Botao 2 Pressionado! Ciclando icone...");
  cycleNextIcon();
}


// =========================================================================
// === Funções Principais (Setup e Loop) ===================================
// =========================================================================

/**
 * @brief Função de configuração principal, executada uma vez quando o ESP32 liga.
 */
void setup() {
  Serial.begin(115200); // Inicia a comunicação serial para debug
  tft.init();           // Inicia o display TFT
  tft.setRotation(1);   // Define a rotação da tela
  tft.setSwapBytes(true);
  
  // Configura os pinos dos botões como entrada com resistor de pull-up interno
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  
  // Limpa a tela e desenha o layout completo definido no arquivo layout.h
  tft.fillScreen(TFT_BLACK);
  drawLayout(tft);
}

/**
 * @brief Função de loop principal, executada repetidamente.
 */
void loop() {
  // LÊ O ESTADO ATUAL DO BOTÃO 1
  int button1State = digitalRead(BUTTON_1_PIN);
  if (button1State == LOW && lastButton1State == HIGH) {
    handleButton1();
  }
  lastButton1State = button1State;

  // LÊ O ESTADO ATUAL DO BOTÃO 2
  int button2State = digitalRead(BUTTON_2_PIN);
  if (button2State == LOW && lastButton2State == HIGH) {
    handleButton2();
  }
  lastButton2State = button2State;

  delay(20); // Delay para debounce e para não sobrecarregar o processador
}