# Atividade 01 – Diagrama em Blocos: Monitor de Temperatura com Alarme e Display 7-Segmentos

## 🎯 Objetivo
Projetar e representar, por meio de um **diagrama em blocos**, a estrutura de hardware de um sistema embarcado destinado ao monitoramento de temperatura ambiente, com alarme sonoro e exibição em display.  

O foco é a **arquitetura de hardware** (blocos e interconexões).

---

## 📚 Contexto e Funcionalidades
O dispositivo deve operar com os seguintes componentes e funções:

- **Sensor de temperatura** (faixa **0 °C a 100 °C**).
- **Alarme sonoro (buzzer)** acionado quando a temperatura ultrapassar um limite configurável (ex.: **60 °C**).
- **Display 7-segmentos de 3 dígitos** para exibir a temperatura em tempo real  
  (ex.: “025”, “060”, “099”).
- **Fonte de alimentação**: Bateria **LiFePO₄ 4S** (~12,8 V nominal; ~14,6 V carga cheia).
- **Microcontrolador** responsável por:
  - Leitura do sensor;
  - Comparação com o limite configurado;
  - Acionamento do alarme;
  - Envio de dados ao display.

📌 Observação:  
O limite pode ser fixo em hardware (jumper/trim) ou definido por interface (botão/encoder).  
Para esta atividade, basta prever no diagrama o bloco **“Interface de Configuração do Limite”** (ex.: botão/encoder).

---

## 🛠️ Passos para a Atividade
1. **Escolha dos componentes**  
   - Selecionar sensor de temperatura adequado;  
   - Definir driver para o display;  
   - Escolher tipo de buzzer.

2. **Elaboração do diagrama em blocos**  
   - Representar todos os módulos e conexões;  
   - Usar **setas claras** para dados e alimentação;  
   - Nomear sinais de comunicação (**ADCx, I²C SCL/SDA, SPI, PWM/GPIO**).  

---

## 📌 Resultado Esperado
Um **diagrama em blocos** completo, mostrando:
- Fonte de alimentação;  
- Microcontrolador;  
- Sensor de temperatura;  
- Alarme sonoro;  
- Display 7-segmentos;  
- Interface de configuração do limite.  
