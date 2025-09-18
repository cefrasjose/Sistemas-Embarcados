# Projeto: Monitor de Temperatura e Umidade com Display TFT e Alarme

**Autor:** (Seu Nome Aqui)
**Data:** 17 de setembro de 2025
**Versão:** 1.0

## Visão Geral

Este documento descreve o diagrama esquemático para um sistema embarcado de monitoramento de temperatura e umidade. O circuito foi projetado para ser alimentado por uma bateria LiFePO4 4S e utiliza um microcontrolador ESP32-S3 para ler dados de um sensor BME280, exibir as informações em um display TFT e acionar um alarme sonoro quando os parâmetros excedem um limite pré-definido.

O foco deste design é a robustez, estabilidade e clareza nas conexões.

## Arquitetura do Circuito

O esquemático está dividido em cinco blocos funcionais principais:

### 1. Fonte de Alimentação

A estratégia de alimentação foi projetada para converter a alta tensão da bateria em níveis de tensão estáveis e seguros para os componentes eletrônicos.

* **Fonte Primária:** 4 células de bateria LiFePO4 em série (`BT1`-`BT4`), fornecendo uma tensão nominal de **~12.8V** (rede `+12V`).
* **Regulador Step-Down (12V -> 5V):** O componente `U5` (LM2596S-5) converte os `+12V` da bateria para `+5V`. Esta rede alimenta o segundo estágio de regulação e os componentes de maior consumo, como o buzzer.
* **Regulador Step-Down (5V -> 3.3V):** O componente `U4` (LM2596S-3.3) converte os `+5V` para `+3.3V`. Esta é a tensão principal de operação para toda a lógica digital, incluindo o MCU, o display e o sensor.
* **Capacitores de Estabilização:** Capacitores eletrolíticos foram adicionados na entrada e saída de ambos os reguladores (`C1`-`C4`) para garantir uma tensão estável e filtrar ruídos, conforme exigido pelo datasheet do LM2596.

### 2. Unidade de Controle Central (MCU)

* **Componente:** `U1` - Módulo ESP32-S3-WROOM-1.
* **Descrição:** O cérebro do projeto. Foi escolhido por sua capacidade de processamento, conectividade Wi-Fi/Bluetooth (para futuras expansões) e grande quantidade de periféricos (I²C, SPI, GPIOs).
* **Alimentação:** O módulo é alimentado pela rede de `+3.3V`. Um capacitor de desacoplamento cerâmico (`C5` - 100nF) está posicionado próximo aos seus pinos de alimentação para garantir a estabilidade do chip.
* **Circuito de Reset:** Um circuito com resistor de pull-up (`R2`) e um botão (opcional) no pino `EN` (Enable) permite que o microcontrolador seja reiniciado manualmente.

### 3. Sensor de Temperatura e Umidade

* **Componente:** `U2` - Módulo BME280.
* **Interface:** I²C (Inter-Integrated Circuit).
    * `I2C_SCL` (Clock): Conectado ao `GPIO9` do ESP32.
    * `I2C_SDA` (Data): Conectado ao `GPIO8` do ESP32.
* **Configuração:** O pino `CSB` está conectado ao `+3V3` para selecionar permanentemente o modo I²C. O pino `SDO` está conectado ao `GND`, configurando o endereço I²C do dispositivo para `0x76`.

### 4. Saídas (Display e Alarme)

* **Display:** `J3` - Conector para um Display LCD TFT de 1.8 polegadas.
    * **Interface:** SPI (Serial Peripheral Interface). As redes são nomeadas de acordo com a função (`TFT_MOSI`, `TFT_SCLK`, `TFT_CS`, `TFT_DC`, `TFT_RST`).
    * **Alimentação:** O display opera com lógica de `+3.3V`.

* **Alarme Sonoro:** `BZ1` - Buzzer Ativo.
    * **Acionamento:** O alarme é controlado pelo `GPIO45` do ESP32.
    * **Circuito Driver:** Um transistor NPN (`Q1`) é usado como uma chave eletrônica. Isso é necessário para amplificar o sinal do pino do MCU, que não consegue fornecer a corrente necessária para acionar o buzzer diretamente. O resistor `R1` (`1kΩ`) limita a corrente na base do transistor.
    * **Alimentação:** O buzzer é alimentado pela rede de `+5V` para garantir maior volume sonoro.

## Lista de Materiais (Bill of Materials - BOM)

| Referência | Componente                                | Quantidade |
| :--------- | :---------------------------------------- | :--------- |
| U1         | Módulo ESP32-S3-WROOM-1                   | 1          |
| U2         | Módulo Sensor BME280                      | 1          |
| U4         | Regulador de Tensão LM2596S-3.3           | 1          |
| U5         | Regulador de Tensão LM2596S-5.0           | 1          |
| Q1         | Transistor NPN (2N2222, BC547 ou similar) | 1          |
| BZ1        | Buzzer Ativo 5V                           | 1          |
| J3         | Conector Header para Display TFT          | 1          |
| R1, R2     | Resistor 1kΩ e 10kΩ                       | 2          |
| C1, C3     | Capacitor Eletrolítico 100µF              | 2          |
| C2, C4     | Capacitor Eletrolítico 220µF              | 2          |
| C5         | Capacitor Cerâmico 100nF                  | 1          |
| BT1-BT4    | Suporte para Bateria / Célula LiFePO4     | 4          |

## Notas do Projeto

* **Níveis de Tensão:** A lógica principal do sistema opera em **3.3V**. Tenha cuidado para não conectar periféricos de 5V diretamente aos pinos do ESP32.
* **Posicionamento dos Componentes:** Para um melhor desempenho, os capacitores de desacoplamento (`C1`-`C5`) devem ser posicionados o mais próximo possível dos pinos de alimentação dos seus respectivos componentes no layout da placa de circuito impresso.
* **Endereço I²C:** Conforme configurado no esquemático, o endereço do sensor BME280 é `0x76`.
