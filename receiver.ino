/**
 * @file receiver.ino
 * @brief Código para recepção de pacotes LoRa com exibição em display OLED.
 * @details Este código configura um receptor LoRa que recebe pacotes e exibe as informações
 *          no display OLED, incluindo o RSSI (Received Signal Strength Indicator) e o conteúdo
 *          do pacote recebido.
 */

// Inclusão de bibliotecas necessárias
#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>               
#include "HT_SSD1306Wire.h"
#include "images.h"

// Definições de frequência e potência de transmissão
#define RF_FREQUENCY                                915000000 // Hz
#define TX_OUTPUT_POWER                             14        // dBm

// Definições de parâmetros LoRa
#define LORA_BANDWIDTH                              0         // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reservado]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Mesmo para Tx e Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Símbolos
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

// Definições de timeout e tamanho do buffer
#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Tamanho do payload

// Variáveis para armazenamento de pacotes e informações de recepção
char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents; // Eventos do rádio

int16_t txNumber; // Número de transmissão
int16_t rssi, rxSize; // RSSI e tamanho do pacote recebido

bool lora_idle = true; // Estado de ociosidade do LoRa

// Configuração do display OLED
static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst

#define DEMO_DURATION 3000
typedef void (*Demo)(void);

int demoMode = 0;
int counter = 1;

/**
 * @brief Função de configuração inicial.
 * @details Configura a comunicação serial, inicializa o rádio LoRa e o display OLED.
 */
void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  txNumber = 0;
  rssi = 0;
  
  // Configuração dos eventos do rádio
  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  Serial.println();
  Serial.println();
  VextON(); // Liga a alimentação do display
  delay(100);

  // Inicialização do display OLED
  display.init();
  display.setFont(ArialMT_Plain_10);
}

/**
 * @brief Liga a alimentação do display OLED.
 */
void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

/**
 * @brief Desliga a alimentação do display OLED.
 */
void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

/**
 * @brief Loop principal do programa.
 * @details Verifica se o LoRa está ocioso e, se estiver, entra no modo de recepção.
 */
void loop() {
  if (lora_idle) {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0); // Inicia a recepção
  }
  Radio.IrqProcess(); // Processa interrupções do rádio
}

/**
 * @brief Função chamada quando um pacote é recebido.
 * @param payload Ponteiro para o payload recebido.
 * @param size Tamanho do payload recebido.
 * @param rssi RSSI do pacote recebido.
 * @param snr SNR do pacote recebido.
 * @details Exibe o pacote recebido e o RSSI no display OLED.
 */
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    display.clear();

    rssi = rssi;
    rxSize = size;
    memcpy(rxpacket, payload, size);
    rxpacket[size] = '\0';
    Radio.Sleep();
    Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n", rxpacket, rssi, rxSize);
    lora_idle = true;

    String myString = String(rxpacket);
    myString = "RSSI:" + String(rssi) + "\n" + myString;

    // Exibe o RSSI e o pacote recebido no display OLED
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, myString);
    display.drawString(10, 128, String(millis()));

    display.display();
}