/**
 * @file sender.ino
 * @brief Código para envio de dados via LoRa com leitura de sensor DHT11.
 * @details Este código configura um dispositivo ESP32 para enviar dados de temperatura e umidade
 *          coletados por um sensor DHT11 via comunicação LoRa.
 * 
 * @author Alunos do curso de Engenharia de Computação, microcontroladores e sistemas embarcados 2025.2
 * @see https://github.com/Maique1915/TempLoRa
 */

// Inclusão de bibliotecas necessárias
#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "DHT.h"

// Definições do sensor DHT
#define DHTPIN 4     ///< Pino digital conectado ao sensor DHT
#define DHTTYPE DHT11   ///< Tipo do sensor DHT (DHT11)

DHT dht(DHTPIN, DHTTYPE); ///< Instância do sensor DHT

// Definições de frequência e potência de transmissão LoRa
#define RF_FREQUENCY 915000000 ///< Frequência do LoRa em Hz
#define TX_OUTPUT_POWER 5      ///< Potência de saída do LoRa em dBm

// Definições de parâmetros LoRa
#define LORA_BANDWIDTH 0            ///< Largura de banda [0: 125 kHz, 1: 250 kHz, 2: 500 kHz]
#define LORA_SPREADING_FACTOR 7      ///< Fator de espalhamento [SF7..SF12]
#define LORA_CODINGRATE 1            ///< Taxa de codificação [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH 8       ///< Comprimento do preâmbulo
#define LORA_SYMBOL_TIMEOUT 0        ///< Tempo limite do símbolo
#define LORA_FIX_LENGTH_PAYLOAD_ON false ///< Define se o payload tem tamanho fixo
#define LORA_IQ_INVERSION_ON false   ///< Define se a inversão de IQ está ativada

// Definições de timeout e tamanho do buffer
#define RX_TIMEOUT_VALUE 1000 ///< Tempo de timeout para recepção de pacotes
#define BUFFER_SIZE 30        ///< Tamanho máximo do buffer de pacotes

// Variáveis para armazenamento de pacotes e informações de transmissão
char txpacket[BUFFER_SIZE]; ///< Buffer para armazenar o pacote de transmissão
char rxpacket[BUFFER_SIZE]; ///< Buffer para armazenar o pacote recebido

double txNumber; ///< Número da transmissão atual

bool lora_idle = true; ///< Estado do LoRa (true = ocioso, false = transmitindo)

static RadioEvents_t RadioEvents; ///< Estrutura para armazenar eventos do rádio

// Protótipos das funções de callback
void OnTxDone(void);
void OnTxTimeout(void);

/**
 * @brief Função de configuração inicial.
 * @details Configura a comunicação serial, inicializa o rádio LoRa e o sensor DHT.
 */
void setup() {
    Serial.begin(115200);
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
    
    txNumber = 0;

    // Configuração dos eventos do rádio
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    
    Radio.Init(&RadioEvents);
    Radio.SetChannel(RF_FREQUENCY);
    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000); 

    Serial.println(F("DHTxx test!")); // Mensagem de inicialização do teste do DHT

    dht.begin(); // Inicializa o sensor DHT
}

/**
 * @brief Loop principal do programa.
 * @details Verifica se o LoRa está ocioso e, se estiver, coleta dados do sensor DHT
 *          e envia os dados via LoRa.
 */
void loop() {
    if (lora_idle == true) {
        delay(1000); // Aguarda 1 segundo entre as leituras

        // Leitura da umidade e temperatura
        float h = dht.readHumidity(); ///< Lê a umidade do sensor
        float t = dht.readTemperature(); ///< Lê a temperatura em Celsius

        // Verifica se a leitura do sensor foi bem-sucedida
        if (isnan(h) || isnan(t)) {
            Serial.println(F("Failed to read from DHT sensor!"));
            return;
        }

        // Formata os dados para envio
        String myString = "UMI: " + String(h) + "%" + "\n" + "TEMP: " + String(t) + "°C";
        Serial.print(myString); // Exibe os dados no monitor serial

        // Prepara o pacote para transmissão
        snprintf(txpacket, BUFFER_SIZE, "%s", myString.c_str());

        Serial.printf("\r\nsending packet \"%s\" , length %d\r\n", txpacket, strlen(txpacket));
  
        Radio.Send((uint8_t *)txpacket, strlen(txpacket)); // Envia o pacote
        lora_idle = false; // Marca o LoRa como ocupado
    }
    Radio.IrqProcess(); // Processa interrupções do rádio
}

/**
 * @brief Função chamada quando a transmissão é concluída com sucesso.
 * @details Marca o LoRa como ocioso após a conclusão da transmissão.
 */
void OnTxDone(void) {
    Serial.println("TX done......");
    lora_idle = true; ///< Marca o LoRa como ocioso
}

/**
 * @brief Função chamada quando ocorre timeout na transmissão.
 * @details Coloca o rádio em modo de sono e marca o LoRa como ocioso.
 */
void OnTxTimeout(void) {
    Radio.Sleep();
    Serial.println("TX Timeout......");
    lora_idle = true; ///< Marca o LoRa como ocioso
}
