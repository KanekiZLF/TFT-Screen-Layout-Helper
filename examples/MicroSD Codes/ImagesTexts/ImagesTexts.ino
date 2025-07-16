/**
 * ===================================================================================
 * === PROJETO DE INTERFACE GRÁFICA PARA TTGO COM LAYOUTS VIA SD CARD ==============
 * ===================================================================================
 * Este código controla um display TFT em um ESP32 (modelo TTGO), lendo layouts
 * de interface (imagens e textos) a partir de um arquivo JSON em um cartão SD.
 * Ele também gerencia a interação do usuário através de dois botões integrados
 * para ciclar entre diferentes ícones de status.
 *
 * Criado com base no trabalho de Luiz Fernando.
 * ===================================================================================
 */

#include <TFT_eSPI.h>      // Biblioteca principal para o controle do display TFT
#include <SPI.h>           // Biblioteca para comunicação SPI (usada pelo SD Card)
#include <SD.h>            // Biblioteca para manipulação do cartão SD
#include <ArduinoJson.h>   // Biblioteca para interpretar os arquivos de layout .JSON

// --- Pinos e Definições ---
// Pinos para o leitor de cartão MicroSD
#define SD_SCK_PIN  15
#define SD_MISO_PIN 12
#define SD_MOSI_PIN 13
#define SD_CS_PIN   33

// Pinos para os botões integrados da placa TTGO
#define BUTTON_1_PIN 0  // Botão 1 (geralmente marcado como BOOT ou IO0 na placa)
#define BUTTON_2_PIN 35 // Botão 2 (geralmente IO35)

// --- Objetos e Variáveis Globais ---
TFT_eSPI tft = TFT_eSPI(); // Objeto principal da biblioteca do display
SPIClass *spiSD = NULL;    // Objeto para a comunicação SPI com o SD Card
bool sdIsInitialized = false; // Flag que indica se o SD Card foi iniciado com sucesso
const uint16_t MASK_COLOR = 0xF81F; // Cor para transparência (Magenta em formato RGB565)

// Variável global para o caminho do arquivo de layout, facilitando a manutenção
String mainLayoutFile = "/SD Files/Layout_TelaInicial.JSON";
String iconsLayoutFile = "/SD Files/Layout_Icons.JSON";
String currentLayoutOnScreen = ""; // Guarda o nome do layout na tela para evitar redesenhos desnecessários

// Variáveis para leitura dos botões
int lastButton1State = HIGH; // Guarda o último estado do botão 1 (HIGH = não pressionado)
int lastButton2State = HIGH; // Guarda o último estado do botão 2

// Variáveis para o ciclo de ícones
const char* iconCycleNames[] = {"bateria", "wi-fi", "usb", "bluetooth", "micro-sd"};
const int   numCycleIcons = 5; // Número total de ícones na lista
int         currentCycleIconIndex = 0; // Índice do ícone atual, começa em 0 ("bateria")


// =========================================================================
// === Funções de Desenho de Baixo Nível ===================================
// =========================================================================

/**
 * @brief Desenha uma imagem .RAW opaca (sem transparência) de forma rápida.
 */
void drawImageFastOpaque(const char *filename, int16_t x, int16_t y, int16_t w, int16_t h) {
  if (!sdIsInitialized) return;
  // Monta o caminho completo para o arquivo de imagem
  String fullPath = "/SD Files/" + String(filename);
  File file = SD.open(fullPath, FILE_READ);
  if (!file) { Serial.printf("Erro: Nao foi possivel abrir a imagem %s\n", fullPath.c_str()); return; }

  tft.setAddrWindow(x, y, w, h); // Define a "janela" no display onde a imagem será desenhada
  const int bufferPixels = 256;  // Tamanho do buffer para leitura em blocos (melhora a velocidade)
  uint8_t  byteBuffer[bufferPixels * 2];
  uint16_t pixelBuffer[bufferPixels];

  while (file.available()) {
    int bytesRead = file.read(byteBuffer, sizeof(byteBuffer)); // Lê um bloco de dados
    if (bytesRead == 0) break;
    int pixelsToPush = bytesRead / 2; // Cada pixel tem 2 bytes (RGB565)
    for (int i = 0; i < pixelsToPush; i++) {
      pixelBuffer[i] = (byteBuffer[i * 2 + 1] << 8) | byteBuffer[i * 2];
    }
    tft.pushColors(pixelBuffer, pixelsToPush); // Envia o bloco de pixels para o display
  }
  file.close();
}

/**
 * @brief Desenha uma imagem .RAW com transparência, pulando a cor definida em MASK_COLOR.
 */
void drawImageWithTransparency(const char *filename, int16_t x, int16_t y, int16_t w, int16_t h) {
  if (!sdIsInitialized) return;
  String fullPath = "/SD Files/" + String(filename);
  File file = SD.open(fullPath, FILE_READ);
  if (!file) { Serial.printf("Erro: Nao foi possivel abrir a imagem %s\n", fullPath.c_str()); return; }

  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      uint8_t pixelBytes[2];
      if (file.read(pixelBytes, 2) != 2) { file.close(); return; }
      uint16_t color = (pixelBytes[1] << 8) | pixelBytes[0];
      if (color != MASK_COLOR) { // Só desenha o pixel se a cor NÃO for a de transparência
        tft.drawPixel(x + i, y + j, color);
      }
    }
  }
  file.close();
}

/**
 * @brief Converte uma string de cor hexadecimal (ex: "#FF0000") para o formato uint16_t (RGB565).
 */
uint16_t hexToRGB565(const char *hex) {
  if (hex == nullptr || hex[0] != '#') return 0;
  long color = strtol(hex + 1, NULL, 16); // Converte a string (ignorando o '#') para um número
  byte r = (color >> 16) & 0xFF; // Extrai o componente Vermelho
  byte g = (color >> 8)  & 0xFF; // Extrai o componente Verde
  byte b = color & 0xFF;         // Extrai o componente Azul
  // Faz a conversão matemática de 24 bits (RGB888) para 16 bits (RGB565)
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}


// =========================================================================
// === Funções de Lógica de Alto Nível =====================================
// =========================================================================

/**
 * @brief (Função Ajudante) Desenha todos os elementos de um layout a partir de um documento JSON já carregado.
 */
void drawAllElementsFromDoc(StaticJsonDocument<2048>& doc) {
  // 1. Desenha as IMAGENS
  JsonArray images = doc["images"].as<JsonArray>();
  for (JsonObject img : images) {
    const char* rawFilename = img["file"]; // O nome do arquivo .RAW
    bool isTransparent = img["transparent"] | false;
    if (isTransparent) {
      drawImageWithTransparency(rawFilename, img["x"], img["y"], img["w"], img["h"]);
    } else {
      drawImageFastOpaque(rawFilename, img["x"], img["y"], img["w"], img["h"]);
    }
  }

  // 2. Desenha os TEXTOS
  tft.setTextDatum(TL_DATUM); // Garante que a referência do texto seja o canto superior esquerdo
  JsonArray texts = doc["texts"].as<JsonArray>();
  for (JsonObject txt : texts) {
    const char* text_content = txt["text"] | "";
    int x = txt["x"];
    int y = txt["y"];
    int font_size = txt["font_size"] | 1;
    const char* color_hex = txt["color_hex"] | "#FFFFFF";
    uint16_t color565 = hexToRGB565(color_hex);
    tft.setTextSize(font_size);
    tft.setTextColor(color565);
    tft.drawString(text_content, x, y);
  }
}

/**
 * @brief Carrega um layout JSON, LIMPA A TELA, e então desenha todos os seus elementos.
 */
void drawLayoutFromJSON(const char *jsonFilename) {
  if (!sdIsInitialized) { Serial.println("Erro: SD nao inicializado."); return; }
  File file = SD.open(jsonFilename, FILE_READ);
  if (!file) { Serial.print("Erro: Layout JSON nao encontrado: "); Serial.println(jsonFilename); return; }
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) { Serial.print("Erro no parsing do JSON: "); Serial.println(error.c_str()); return; }
  
  tft.fillScreen(TFT_BLACK); // Limpa a tela antes de desenhar
  drawAllElementsFromDoc(doc);
}

/**
 * @brief Redesenha todos os elementos de um layout sobre o conteúdo atual da tela (NÃO limpa a tela).
 */
void refreshLayout(const char *jsonFilename) {
  if (!sdIsInitialized) { Serial.println("Erro: SD nao inicializado."); return; }
  File file = SD.open(jsonFilename, FILE_READ);
  if (!file) { Serial.print("Erro: Layout JSON nao encontrado: "); Serial.println(jsonFilename); return; }
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) { Serial.print("Erro no parsing do JSON: "); Serial.println(error.c_str()); return; }
  
  drawAllElementsFromDoc(doc); // Chama a função ajudante, sem limpar a tela
}

/**
 * @brief Desenha uma lista de elementos específicos (imagens ou textos) pelo nome, sem limpar a tela.
 */
void drawElementsByName(const char* elementNames[], int numElements, const char* jsonFilename) {
  if (!sdIsInitialized) { Serial.println("Erro: SD nao inicializado."); return; }
  File file = SD.open(jsonFilename, FILE_READ);
  if (!file) { Serial.print("Erro: Layout JSON nao encontrado: "); Serial.println(jsonFilename); return; }
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) { Serial.print("Erro no parsing do JSON: "); Serial.println(error.c_str()); return; }

  JsonArray images = doc["images"].as<JsonArray>();
  JsonArray texts = doc["texts"].as<JsonArray>();

  for (int i = 0; i < numElements; i++) {
    const char* nameToFind = elementNames[i];
    bool elementFound = false;
    // Procura na lista de imagens
    for (JsonObject img : images) {
      const char* currentName = img["name"];
      if (currentName && strcmp(currentName, nameToFind) == 0) {
        const char* rawFilename = img["file"];
        bool isTransparent = img["transparent"] | false;
        if (isTransparent) {
          drawImageWithTransparency(rawFilename, img["x"], img["y"], img["w"], img["h"]);
        } else {
          drawImageFastOpaque(rawFilename, img["x"], img["y"], img["w"], img["h"]);
        }
        elementFound = true;
        break;
      }
    }
    if (elementFound) continue;
    // Se não achou, procura na lista de textos
    tft.setTextDatum(TL_DATUM);
    for (JsonObject txt : texts) {
      const char* currentName = txt["name"];
      if (currentName && strcmp(currentName, nameToFind) == 0) {
        const char* text_content = txt["text"] | "";
        int x = txt["x"];
        int y = txt["y"];
        int font_size = txt["font_size"] | 1;
        const char* color_hex = txt["color_hex"] | "#FFFFFF";
        uint16_t color565 = hexToRGB565(color_hex);
        tft.setTextSize(font_size);
        tft.setTextColor(color565);
        tft.drawString(text_content, x, y);
        elementFound = true;
        break;
      }
    }
    if (!elementFound) {
      Serial.printf("Aviso: Elemento '%s' nao encontrado no layout '%s'\n", nameToFind, jsonFilename);
    }
  }
}

// =========================================================================
// === Funções de Ação para cada Botão =====================================
// =========================================================================

/**
 * @brief Avança para o próximo ícone no ciclo e o desenha na tela.
 */
void cycleNextIcon() {
  // Limpa a Tela para redesenhar
  tft.fillScreen(TFT_BLACK);
  
  // Crie um array contendo o nome do(s) elemento(s) que você quer desenhar.
  const char* textToDraw[] = {"text_1"};

  // Chame a função, passando o array, o número de itens (neste caso, 1), e o arquivo de layout.
  drawElementsByName(textToDraw, 1, iconsLayoutFile.c_str());

  // A operação de módulo (%) é uma forma elegante de fazer o ciclo.
  // Ex: (0+1)%4=1, (1+1)%4=2, (2+1)%4=3, (3+1)%4=0
  currentCycleIconIndex = (currentCycleIconIndex + 1) % numCycleIcons;

  const char* iconToDraw[] = { iconCycleNames[currentCycleIconIndex] };
  Serial.printf("Ciclando para o icone: %s\n", iconToDraw[0]);
  
  // Desenha apenas o ícone atual, usando a variável global para o caminho do arquivo de layout
  drawElementsByName(iconToDraw, 1, iconsLayoutFile.c_str());
}

// Botão 1: Quando pressionado, chama a função de ciclo de ícones.
void handleButton1() {
  Serial.println("Botao 1 Pressionado! Ciclando icone...");
  cycleNextIcon();
}

// Botão 2: Também chama a mesma função, fazendo os dois botões terem a mesma ação.
void handleButton2() {
  Serial.println("Botao 2 Pressionado! Ciclando icone...");
  cycleNextIcon();
}


// =========================================================================
// === Funções Principais (Setup e Loop) ===================================
// =========================================================================

/**
 * @brief Inicializa o leitor de cartão SD e reporta o status no display.
 */
void initSDCard() {
  tft.fillScreen(TFT_BLACK); tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setTextSize(2);
  tft.setCursor(10, 10); tft.println("Inicializando SD...");
  if (spiSD == NULL) { spiSD = new SPIClass(HSPI); spiSD->begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN); }
  if (!SD.begin(SD_CS_PIN, *spiSD)) {
    tft.println("SD Card nao encontrado!"); sdIsInitialized = false; delay(2000);
  } else {
    tft.println("SD OK!"); sdIsInitialized = true; delay(1000);
  }
}

/**
 * @brief Função de configuração principal, executada uma vez quando o ESP32 liga.
 */
void setup() {
  Serial.begin(115200); // Inicia a comunicação serial para debug
  tft.init();           // Inicia o display TFT
  tft.setRotation(1);   // Define a rotação da tela (0-3)
  tft.setSwapBytes(false); // Importante para as cores dos arquivos .RAW
  
  // Configura os pinos dos botões como entrada com resistor de pull-up interno.
  // O pino fica em HIGH e vai para LOW quando o botão é pressionado.
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  
  initSDCard(); // Tenta inicializar o cartão SD
  
  if (sdIsInitialized) {
    // Se o SD foi iniciado com sucesso, desenha o layout inicial
    drawLayoutFromJSON(mainLayoutFile.c_str());
    currentLayoutOnScreen = mainLayoutFile;
  }
}

/**
 * @brief Função de loop principal, executada repetidamente.
 */
void loop() {
  // LÊ O ESTADO ATUAL DO BOTÃO 1
  int button1State = digitalRead(BUTTON_1_PIN);
  // Checa se o botão FOI PRESSIONADO (o estado mudou de HIGH para LOW)
  if (button1State == LOW && lastButton1State == HIGH) {
    handleButton1(); // Chama a função de ação do botão 1
  }
  lastButton1State = button1State; // Atualiza o último estado para a próxima verificação

  // LÊ O ESTADO ATUAL DO BOTÃO 2
  int button2State = digitalRead(BUTTON_2_PIN);
  if (button2State == LOW && lastButton2State == HIGH) {
    handleButton2(); // Chama a função de ação do botão 2
  }
  lastButton2State = button2State;

  // Pequeno delay para evitar leituras instáveis (debounce) e sobrecarga do processador
  delay(20); 
}