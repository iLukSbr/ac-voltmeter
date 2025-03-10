#include <SPI.h>
#include <MD_MAX72XX.h>

// Definição dos pinos para o ESP32 (ajuste conforme seu esquema)
#define ADS1288_CS  5  // Chip Select para ADS1288 (GPIO5, exemplo)
#define MAX7219_CS  15 // Chip Select para MAX7219 (GPIO15, exemplo)
#define MAX7219_DIN 23 // Data In para MAX7219 (MOSI, GPIO23)
#define MAX7219_CLK 18 // Clock para MAX7219 (SCLK, GPIO18)

// Definição do hardware para MD_MAX72XX (usando SPI)
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 1  // 1 módulo MAX7219 para 4 dígitos

// Criar instância do display MAX7219
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, MAX7219_CS, MAX_DEVICES);

// Configurações do ADS1288
const uint32_t ADS1288_FREQ = 1000000; // Frequência SPI para ADS1288 (1 MHz)
const float VREF = 2.5; // Referência de tensão interna do ADS1288 (2.5V, ajustável)
const float GAIN = 1.0; // Ganho do ADS1288 (ajuste conforme configuração)
const float R1 = 1000000.0; // Resistor superior do divisor (1MΩ, ajuste conforme esquema)
const float R2 = 10000.0;   // Resistor inferior do divisor (10kΩ, ajuste conforme esquema)

// Variáveis globais
float tensao = 0.0;

void setup() {
  Serial.begin(115200);
  SPI.begin(); // Inicializa SPI para ESP32

  // Configurar SPI para ADS1288
  SPI.setFrequency(ADS1288_FREQ);
  SPI.setDataMode(SPI_MODE1); // Modo SPI típico para ADS1288
  pinMode(ADS1288_CS, OUTPUT);
  digitalWrite(ADS1288_CS, HIGH); // Desativa inicialmente

  // Inicializar MAX7219
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 5); // Brilho do display (0-15)
  mx.control(MD_MAX72XX::TEST, false);  // Desativa teste
  mx.clear(); // Limpa o display

  // Configurar o ADS1288 (simplificado, ajuste conforme datasheet)
  configurarADS1288();
}

void loop() {
  // Ler dados do ADS1288
  uint32_t adcValue = lerADS1288();
  // Converter para tensão (ajuste conforme seu divisor de tensão e ganho)
  tensao = calcularTensao(adcValue);

  // Exibir no display MAX7219 (4 dígitos, com 1 casa decimal)
  exibirTensao(tensao);

  // Imprimir no Serial Monitor para depuração
  Serial.print("Tensão medida: ");
  Serial.print(tensao, 1); // 1 casa decimal
  Serial.println(" V");

  delay(100); // Ajuste o intervalo de leitura
}

// Função para configurar o ADS1288 (simplificada)
void configurarADS1288() {
  digitalWrite(ADS1288_CS, LOW);
  // Enviar comandos de configuração (exemplo básico, consulte datasheet ADS1288)
  // Configurar modo contínuo, ganho, etc.
  uint8_t config[] = {0x01, 0x00}; // Exemplo, ajuste conforme datasheet
  SPI.transfer(config, 2);
  digitalWrite(ADS1288_CS, HIGH);
}

// Função para ler dados do ADS1288
uint32_t lerADS1288() {
  digitalWrite(ADS1288_CS, LOW);
  uint32_t value = 0;
  // Ler 24 bits (3 bytes) do ADS1288 via SPI
  value = SPI.transfer(0xFF) << 16;
  value |= SPI.transfer(0xFF) << 8;
  value |= SPI.transfer(0xFF);
  digitalWrite(ADS1288_CS, HIGH);
  // Ajustar para valor de 24 bits (ignorar bits superiores se necessário)
  value = value & 0xFFFFFF; // Máscara para 24 bits
  return value;
}

// Função para calcular a tensão com base no valor ADC
float calcularTensao(uint32_t adcValue) {
  // Converter o valor ADC para tensão (0 a 2.5V, supondo 24 bits)
  float voltage = (adcValue * VREF) / (pow(2, 24) - 1);
  // Ajustar para a tensão real após o divisor (Vout = Vin * R2/(R1+R2))
  float vin = voltage * (R1 + R2) / R2 / GAIN;
  return vin;
}

// Função para exibir a tensão no display MAX7219
void exibirTensao(float tensao) {
  // Limitar a 4 dígitos (ex.: 150.0V -> "1500" ou "15.0" ajustado)
  int valorDisplay = (int)(tensao * 10); // Multiplica por 10 para 1 casa decimal
  if (valorDisplay > 9999) valorDisplay = 9999; // Limitar a 4 dígitos

  // Dividir em dígitos (milhares, centenas, dezenas, unidades)
  int digitos[4];
  digitos[0] = (valorDisplay / 1000) % 10; // Milhares
  digitos[1] = (valorDisplay / 100) % 10;   // Centenas
  digitos[2] = (valorDisplay / 10) % 10;    // Dezenas
  digitos[3] = valorDisplay % 10;          // Unidades (decimal implícito)

  // Exibir no display (posição 0 é o dígito mais à esquerda)
  for (int i = 0; i < 4; i++) {
    mx.setChar(0, 3 - i, digitos[i] + '0'); // Converte para caractere
  }
  // Adicionar ponto decimal (ex.: para "15.0", adicionar ponto após o 0)
  if (tensao >= 10 && tensao < 100) {
    mx.setPoint(0, 1, true); // Ponto decimal após o segundo dígito (15.0)
  } else if (tensao < 10) {
    mx.setPoint(0, 0, true); // Ponto decimal após o primeiro dígito (5.0)
  }
  mx.update();
}