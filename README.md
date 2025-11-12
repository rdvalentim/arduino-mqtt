# 🧠 Detecção de Vazamento de Gás em Sala Técnica com MQTT

Projeto desenvolvido como parte do Trabalho 2 da disciplina de Sistemas Embarcados, com o objetivo de evoluir o sistema de detecção de vazamentos de gás, adicionando conectividade IoT via protocolo **MQTT** utilizando o **ESP32**.

O sistema monitora uma concentração simulada de gás (via potenciômetro), classifica o estado em **Normal**, **Atenção** e **Emergência**, e publica os dados em tempo real para um **broker MQTT**, permitindo também o recebimento de comandos remotos.

---

## ⚙️ Estrutura do Projeto

```
├── work.ino          # Código-fonte principal do ESP32
├── secrets.h         # Credenciais Wi-Fi e broker (NÃO subir senhas reais)
├── README.md         # Este documento
```

---

## 🧩 Bibliotecas Utilizadas

Certifique-se de instalar as seguintes bibliotecas na **Arduino IDE**:

- `WiFi.h` (padrão da placa ESP32)  
- `PubSubClient.h` – cliente MQTT  
- `ArduinoJson.h` – para manipulação dos objetos JSON  

Para instalar, vá em **Sketch → Incluir Biblioteca → Gerenciar Bibliotecas** e procure por cada uma delas.

---

## 🔧 Compilação e Upload

1. **Placa:** selecione **ESP32 Dev Module** (ou modelo compatível).  
2. **Porta:** selecione a porta COM correspondente ao seu dispositivo.  
3. **Baud rate recomendado:** 115200.  
4. Compile e faça o upload diretamente na IDE do Arduino.

---

## 🌐 Configuração de Wi-Fi e Broker

As credenciais de rede e informações do broker são definidas no arquivo `secrets.h`.

**⚠️ Importante:** nunca exponha senhas reais em repositórios públicos.  
Use valores genéricos, como no exemplo abaixo:

```cpp
#define WIFI_SSID     "MinhaRedeWiFi"
#define WIFI_PASSWORD "********"
#define MQTT_BROKER   "test.mosquitto.org"
#define MQTT_PORT     1883
#define MQTT_USER     ""
#define MQTT_PASSWORD ""
```

---

## 🗂️ Estrutura dos Tópicos MQTT

Padrão utilizado pela disciplina:

```
iot/riodosul/si/BSN22025T26F8/cell/3/device/c3-ramon-gustavo/
    ├── state
    ├── telemetry
    ├── event
    ├── cmd
    └── config
```

---

## 💬 Exemplos de JSON

### **Publicação de Telemetria**
```json
{
  "ts": 1739812345,
  "cellId": 3,
  "devId": "c3-ramon-gustavo",
  "metrics": { "ppm": 485 },
  "status": "atencao",
  "units": { "ppm": "partes por milhão" },
  "thresholds": { "normal": 300, "critico": 600 }
}
```

### **Comando: Get Status**
```json
{ "action": "get_status" }
```

### **Comando: Alterar Thresholds**
```json
{ 
  "action": "set_thresholds", 
  "data": { "normal": 250, "critico": 550 } 
}
```

---

## 🚀 Envio de Comandos (via Terminal)

Exemplo de envio utilizando **Mosquitto**:

```bash
mosquitto_pub -h test.mosquitto.org -t "iot/riodosul/si/BSN22025T26F8/cell/3/device/c3-ramon-gustavo/cmd"   -m '{"action":"get_status"}'
```

Ou para ajustar os limites:

```bash
mosquitto_pub -h test.mosquitto.org -t "iot/riodosul/si/BSN22025T26F8/cell/3/device/c3-ramon-gustavo/cmd"   -m '{"action":"set_thresholds", "data":{"normal":250,"critico":550}}'
```

---

## 🧪 Checklist de Testes

| Teste                                | Descrição | Resultado Esperado |
|-------------------------------------|------------|---------------------|
| 🔵 **Faixa Normal (<300 ppm)**      | LED verde aceso; sem buzzer; JSON com `"status":"normal"` | OK |
| 🟡 **Faixa Atenção (300–600 ppm)**  | LED amarelo aceso; sem buzzer; JSON com `"status":"atencao"` | OK |
| 🔴 **Faixa Emergência (>600 ppm)**  | LED vermelho aceso; buzzer ativo; JSON com `"status":"emergencia"` | OK |
| 📡 **Comando get_status**           | Envio de comando MQTT; resposta imediata no tópico `event` | OK |
| 🔁 **Reconexão automática**         | Interromper Wi-Fi e reconectar; o dispositivo deve retomar publicações | OK |

---

## 📚 Créditos

Desenvolvido por  
**Gustavo Verdi** e **Ramon Diego Valentim**  
Sistemas de Informação – UNIDAVI (2025)
