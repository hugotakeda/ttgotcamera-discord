```markdown
# 🚨 Sistema de Monitoramento com TTGO T-Camera + Notificações em Tempo Real

[![Arduino](https://img.shields.io/badge/Arduino-Compatible-green)](https://www.arduino.cc/)
[![ESP32](https://img.shields.io/badge/ESP32-TTGO_T--Camera-blue)](https://www.lilygo.cc/products/t-camera)
[![Node.js](https://img.shields.io/badge/Node.js-18%2B-brightgreen)](https://nodejs.org/)

Projeto baseado em IoT que utiliza um **TTGO T-Camera** para detecção de movimentos com sensor PIR, integrando notificações instantâneas no Discord e um dashboard web atualizado em tempo real via WebSocket.

> 🔍 **Inspirado no projeto**: [TTGO Security Camera with PIR Motion Sensor](https://robotzero.one/ttgo-security-camera-pir/) por *RobotZero*

---

## 🌟 Funcionalidades

- Detecção de movimento com registro preciso de data/hora (NTP)
- Notificações push no Discord com menção `@everyone`
- Dashboard web responsivo com histórico de eventos
- Comunicação em tempo real via WebSocket
- Compatível com dispositivos móveis

## 🛠 Hardware Necessário

| Componente | Especificações |
|------------|----------------|
| Placa | TTGO T-Camera (ESP32) |
| Sensor | PIR Motion Sensor (HC-SR501) |
| Conexão | Wi-Fi 2.4GHz |

## 📦 Estrutura do Projeto

```
.
├── Arduino/               # Código para TTGO T-Camera
│   └── ArduCam.ino        # Lógica principal com PIR e notificações
├── server/                # Servidor Node.js
│   ├── server.js          # Backend com WebSocket
│   └── index.html         # Interface web
└── README.md              # Documentação
```

## 🚀 Configuração Rápida

1. **Hardware**:
   - Conecte o sensor PIR ao pino `GPIO33` do TTGO T-Camera

2. **Arduino**:
   ```cpp
   // Configure no ArduCam.ino:
   const char* ssid = "SUA_REDE_WIFI";
   const char* password = "SENHA_WIFI";
   const char* discordWebhook = "SEU_WEBHOOK_DISCORD";
   ```

3. **Node.js**:
   ```bash
   npm install express ws
   node server.js
   ```

4. **Acesse**:
   - Dashboard: `http://localhost:3000`
   - Endpoint de logs: `POST /log`

## 🤝 Créditos e Referências

- Projeto que faz uso do PIR com TTGO que me ajudou na criação do projeto: [RobotZero.one](https://robotzero.one/ttgo-security-camera-pir/)

## 📄 Licença

MIT License - Consulte o arquivo [LICENSE](LICENSE) para detalhes.
