
# TFT Screen Layout Helper

![Badge da Versão](https://img.shields.io/badge/versão-1.2-blue)
![Badge da Licença](https://img.shields.io/badge/licença-MIT-green)
![Badge de Plataforma](https://img.shields.io/badge/plataforma-Windows-informational)

Uma ferramenta gráfica desenvolvida para acelerar e simplificar a criação de layouts de tela para microcontroladores (como ESP32 e outros) que utilizam displays TFT. Diga adeus ao "chute" de coordenadas e posições! Crie, posicione e exporte seus elementos visuais de forma rápida e intuitiva.

![Imagem Principal](https://img001.prntscr.com/file/img001/8M35Ay5VScahemh0G7GrUw.png)

## ✨ Funcionalidades Principais

- **Canvas Visual:** Uma representação fiel da tela do seu display.
- **Dimensões Configuráveis:** Ajuste a largura e altura do canvas para corresponder exatamente ao seu display.
- **Importação de Imagens:** Adicione imagens (`.png`, `.jpg`, etc.) diretamente ao layout.
- **Adição de Texto:** Insira elementos de texto, configure tamanho da fonte e cor.
- **Manipulação Drag-and-Drop:** Arraste e solte os elementos para posicioná-los com precisão.
- **Redimensionamento Inteligente:** Altere o tamanho das imagens, com a opção de manter a proporção original.
- **Gerenciamento de Elementos:** Uma lista exibe todos os elementos na tela, permitindo fácil seleção, edição e exclusão.
- **Geração de Código Automática:** Exporte seu layout final de duas maneiras:
  1. **Memória Interna:** Gera um arquivo de cabeçalho C++ (`.h`) com todos os dados de imagem e texto prontos para serem incluídos no seu código Arduino/PlatformIO.
  2. **Cartão MicroSD:** Gera arquivos de imagem no formato `.RAW` (RGB565) e um arquivo `layout.json` para carregar dinamicamente os elementos a partir de um SD.
- **Salvar e Carregar Projetos:** Guarde todo o seu trabalho em um arquivo de projeto (`.json`) para continuar depois.
- **Interface Amigável:** Suporte a temas (Light/Dark) e múltiplos idiomas (Português/Inglês).

## 🚀 Como Usar

Para um guia visual completo, assista ao vídeo tutorial no YouTube. Clique na imagem abaixo:

[![Assista ao Vídeo Tutorial](https://img.youtube.com/vi/ID_DO_SEU_VIDEO/0.jpg)](https://www.youtube.com/watch?v=ID_DO_SEU_VIDEO)

### 1. Configuração Inicial da Tela

Ao abrir o programa, a primeira coisa a fazer é definir o tamanho da sua tela TFT.

1. Insira a **Largura** e **Altura** em pixels nos campos correspondentes.
2. Clique em **"Atualizar Tamanho da Tela"**. O canvas será redimensionado.

![Tela de Config](https://img001.prntscr.com/file/img001/hHWvA1-pTWa2CG7ybprsBQ.png)

### 2. Adicionando e Manipulando Elementos

- **Adicionar:** Use os botões **"Importar Imagem"** e **"Adicionar Texto"** para popular seu layout.
- **Manipular:** Clique e arraste os elementos no canvas. Use o painel à direita para selecionar, redimensionar, editar e excluir os itens.

![Texto](https://img001.prntscr.com/file/img001/-lCA4lIvQ6-04_INP6Ih-A.png)

### 3. Gerando os Arquivos Finais

Escolha o "Tipo de Memória de Saída" e clique em **"Gerar Código / Arquivos"**.

#### Opção 1: Memória Interna

Gera um único arquivo de cabeçalho C++ (`layout.h`) com tudo embutido.

**Como usar no seu código (Arduino IDE / PlatformIO):**

1. Salve o arquivo gerado na pasta do seu projeto.
2. Inclua o arquivo no seu sketch (`#include "layout.h"`).
3. Chame a função `drawLayout(tft);` dentro do seu `setup()` ou de outra função de desenho.

```cpp
#include <TFT_eSPI.h>
#include "layout.h" // <-- Seu arquivo gerado

TFT_eSPI tft = TFT_eSPI();

void setup() {
  tft.begin();
  tft.fillScreen(TFT_BLACK);
  //tft.setSwapBytes(true); // Exemplo caso estivesse desenhando com SD e Memoria Interna, é necessario mudar isso 
  drawLayout(tft); // Desenha tudo com uma única chamada!
}

void loop() { }
```

![MyEsp32](https://img001.prntscr.com/file/img001/FbzcnETcSgani3OHEyNl2g.png)

#### Opção 2: Micro SD

Gera múltiplos arquivos (.RAW e .JSON) para serem colocados em um cartão SD.

**Como usar no seu projeto:**

- Copie todos os arquivos gerados para a raiz de um cartão MicroSD.
- Use um código no seu microcontrolador para ler o arquivo JSON e desenhar cada elemento. Você precisará da biblioteca ArduinoJson.

**Exemplo de código para ESP32:**

```cpp
/*
  Exemplo de como carregar e desenhar um layout a partir de um cartão MicroSD.
  Este código demonstra como ler um arquivo JSON de um cartão SD,
  interpretar os dados para desenhar imagens e textos em uma tela TFT.

  REQUISITOS:
  - Biblioteca ArduinoJson: https://arduinojson.org/ (Instale pelo Library Manager)
  - Biblioteca TFT_eSPI: (Geralmente instalada manualmente ou via PlatformIO)
  - Cartão SD formatado (FAT32) com os arquivos de imagem .RAW e o arquivo de layout .JSON na raiz.
  - Pinos do cartão SD corretamente definidos para a sua placa de desenvolvimento (ESP32, etc.).
*/

// --- INCLUSÃO DE BIBLIOTECAS ---
#include <TFT_eSPI.h>       // Biblioteca principal para controle da tela TFT
#include <SD.h>             // Biblioteca para comunicação com o cartão SD
#include <SPI.h>            // Biblioteca para o protocolo de comunicação SPI (usado pelo SD e TFT)
#include <ArduinoJson.h>    // Biblioteca para manipular (ler e escrever) dados no formato JSON

// --- OBJETOS E DEFINIÇÕES GLOBAIS ---

// Cria uma instância do objeto da tela TFT. É através dele que todos os comandos de desenho serão chamados.
TFT_eSPI tft = TFT_eSPI();

// Define os pinos do microcontrolador (ex: ESP32) que estão conectados ao leitor de cartão MicroSD.
// Estes pinos podem variar dependendo da sua placa e da sua fiação.
#define SD_SCK_PIN  18  // Pino do Clock (Serial Clock)
#define SD_MISO_PIN 19  // Pino MISO (Master In, Slave Out)
#define SD_MOSI_PIN 23  // Pino MOSI (Master Out, Slave In)
#define SD_CS_PIN   5   // Pino CS (Chip Select), usado para selecionar o dispositivo SD no barramento SPI.

// --- FUNÇÕES AUXILIARES ---

/**
 * @brief Converte uma cor no formato string hexadecimal (ex: "#FF0000" para vermelho)
 * para o formato de 16 bits RGB565, que é o formato de cor usado pela biblioteca TFT_eSPI.
 *
 * @param hex Ponteiro para a string contendo a cor hexadecimal (ex: "#RRGGBB").
 * @return A cor convertida para o formato uint16_t (RGB565).
 */
uint16_t hexToRGB565(const char *hex) {
  // Converte a string hexadecimal (ignorando o '#') para um número inteiro longo.
  long color = strtol(hex + 1, NULL, 16);
  // Extrai os componentes de cor Vermelho (R), Verde (G) e Azul (B) do número inteiro.
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8)  & 0xFF;
  uint8_t b = color & 0xFF;
  // Usa a função da biblioteca TFT para converter os componentes R, G, B de 8 bits para o formato de 16 bits (RGB565).
  return tft.color565(r, g, b);
}

// --- FUNÇÃO PRINCIPAL DE DESENHO ---

/**
 * @brief Lê um arquivo de layout JSON do cartão SD e desenha os elementos na tela TFT.
 *
 * @param layoutFile O nome do arquivo JSON (com o caminho, ex: "/meu_layout.json") a ser carregado.
 */
void drawLayoutFromSD(const char* layoutFile) {
  // Tenta abrir o arquivo de layout no modo de leitura.
  File file = SD.open(layoutFile);
  if (!file) { // Se o arquivo não puder ser aberto, exibe uma mensagem de erro na tela e para a execução da função.
    tft.setCursor(0, 0); // Posiciona o cursor no canto superior esquerdo
    tft.println("Falha ao abrir o layout.json");
    return;
  }

  // Cria um documento JSON dinâmico na memória para armazenar os dados do arquivo.
  // O tamanho (2048 bytes) deve ser suficiente para o seu arquivo JSON. Ajuste se necessário.
  DynamicJsonDocument doc(2048);
  // Faz a "desserialização", ou seja, lê o texto do arquivo e o converte para um objeto JSON na memória.
  DeserializationError error = deserializeJson(doc, file);
  // Fecha o arquivo assim que terminar de ler para liberar recursos.
  file.close();

  // Verifica se ocorreu algum erro durante a conversão do JSON.
  if (error) {
    tft.setCursor(0, 0);
    tft.println("Falha ao processar o JSON.");
    tft.print("Erro: ");
    tft.println(error.c_str()); // Imprime o tipo de erro para ajudar na depuração.
    return;
  }

  // --- Processa e desenha as IMAGENS ---
  // Acessa o array "images" dentro do objeto JSON.
  JsonArray images = doc["images"];
  // Itera sobre cada elemento (objeto) dentro do array de imagens.
  for (JsonObject img : images) {
    // Extrai os valores de cada chave do objeto JSON da imagem.
    const char* imgFile = img["file"]; // Nome do arquivo .RAW da imagem
    int x = img["x"];                  // Posição X na tela
    int y = img["y"];                  // Posição Y na tela
    int w = img["w"];                  // Largura da imagem
    int h = img["h"];                  // Altura da imagem

    // Abre o arquivo da imagem .RAW correspondente.
    File imageRaw = SD.open(imgFile);
    if (imageRaw) { // Se o arquivo da imagem foi aberto com sucesso...
      // A função pushImage é otimizada para desenhar imagens a partir de um fluxo de dados (como um arquivo).
      // A biblioteca TFT_eSPI precisa de um programa externo para converter imagens (PNG, JPG) para o formato .RAW.
      tft.pushImage(x, y, w, h, imageRaw);
      // Fecha o arquivo da imagem.
      imageRaw.close();
    }
  }

  // --- Processa e desenha os TEXTOS ---
  // Acessa o array "texts" dentro do objeto JSON.
  JsonArray texts = doc["texts"];
  // Itera sobre cada elemento (objeto) dentro do array de textos.
  for (JsonObject txt : texts) {
    // Extrai os valores de cada chave do objeto JSON do texto.
    const char* text = txt["text"];             // O conteúdo do texto a ser exibido
    int x = txt["x"];                           // Posição X do ponto de referência do texto
    int y = txt["y"];                           // Posição Y do ponto de referência do texto
    int fontSize = txt["font_size"];            // O tamanho da fonte (escala)
    const char* colorHex = txt["color_hex"];    // A cor do texto no formato hexadecimal

    // Define as propriedades do texto na biblioteca TFT antes de desenhar.
    tft.setTextSize(fontSize);                             // Define o tamanho da fonte
    tft.setTextColor(hexToRGB565(colorHex));               // Define a cor, usando nossa função auxiliar
    // A biblioteca TFT_eSPI pode usar fontes diferentes. Aqui, usamos a padrão.
    // A função drawString desenha o texto na tela nas coordenadas especificadas.
    tft.drawString(text, x, y);
  }
}


// --- FUNÇÃO DE CONFIGURAÇÃO (Executada uma vez no início) ---
void setup() {
  // Inicia a comunicação serial para depuração (opcional, mas recomendado).
  Serial.begin(115200);

  // Inicia e configura a tela TFT.
  tft.begin();
  // Preenche toda a tela com a cor preta para limpá-la.
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(false); // Importante para as cores dos arquivos .RAW Use false, somente se os arquivos forem .RAW, se for memoria interna use true

  // Inicia a comunicação com o cartão SD, especificando o pino Chip Select (CS).
  // A biblioteca SD usará os pinos SPI padrão definidos para a placa.
  if (!SD.begin(SD_CS_PIN)) {
    // Se a inicialização falhar, informa na tela e no monitor serial.
    tft.setCursor(0, 0);
    tft.println("Falha ao iniciar o cartao SD!");
    Serial.println("Falha ao iniciar o cartao SD!");
    return; // Para a execução do setup, pois nada mais funcionará.
  }

  // Chama a função principal para desenhar o layout a partir do arquivo especificado.
  drawLayoutFromSD("/Layout_meu_projeto.JSON");
}

// --- FUNÇÃO DE LOOP (Executada repetidamente após o setup) ---
void loop() {
  // O loop está vazio porque, neste exemplo, o layout é desenhado apenas uma vez
  // durante a inicialização no setup(). Se você precisasse de atualizações
  // contínuas na tela (como um relógio ou um medidor), o código para isso iria aqui.
}
```
![MyEsp32](https://img001.prntscr.com/file/img001/UWDGQ67IS8aV4FmPCYYWiA.png)

---

## 🔧 Requisitos do Sistema

- **Sistema Operacional:** Windows (64-bit)  
- Nenhuma instalação de Python ou bibliotecas é necessária.

## 🏃 Como Executar o Programa
1. Baixe o arquivo `.exe` da versão mais recente.
2. Execute o arquivo `TFT Screen Layout Helper.exe`.

## 👤 Autor

**Luiz F. R. Pimentel** - [GitHub](https://github.com/KanekiZLF)

Sinta-se à vontade para contribuir, reportar bugs ou sugerir novas funcionalidades!
