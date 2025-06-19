🚨 Sistema de Monitoramento com TTGO T-Camera e Webcam

Um projeto de IoT que utiliza o TTGO T-Camera com sensor PIR para detecção de movimentos, integrado a uma webcam Logitech C920 para captura de fotos. O sistema envia notificações em tempo real ao Discord e exibe logs e gráficos em um dashboard web responsivo. O TTGO também cria um Access Point Wi-Fi para acesso local ao dashboard.

🔍 Inspirado no projeto: TTGO Security Camera with PIR Motion Sensor por RobotZero


🌟 Funcionalidades

Detecção de Movimento: Usa o sensor PIR (GPIO 33) do TTGO T-Camera para detectar movimentos.
Captura de Fotos: Integra uma webcam Logitech C920 no servidor (Mac) para capturar imagens de alta qualidade.
Notificações no Discord: Envia alertas com menção @everyone e fotos com timestamp no formato DD/MM/YYYY às HH:MM:ss (horário de São Paulo, Brasil).
Dashboard Web Responsivo:
Exibe histórico de detecções com fotos, ordenado do mais recente ao mais antigo.
Gráficos lado a lado (detecções por hora e últimos 7 dias) usando Chart.js.
Contadores para total de detecções, hoje, semana e mês.
Botões para atualizar, exportar (JSON) e limpar logs.
Paginação para navegação no histórico.


Persistência de Dados: Logs e fotos salvos em logs.json e pasta photos/.
Access Point: O TTGO cria um Wi-Fi (SSID: Takeda, senha: vasscl62) para acesso ao dashboard via http://192.168.4.1.
Compatibilidade Móvel: Dashboard otimizado para computadores e celulares.

🛠 Requisitos
Hardware



Componente
Especificações



Placa
TTGO T-Camera (ESP32)


Sensor
PIR Motion Sensor (integrado, GPIO 33)


Webcam
Logitech C920 (conectada ao Mac)


Conexão
Wi-Fi 2.4GHz (para conexão com o servidor)


Software



Componente
Versão



Arduino IDE
1.8.x ou superior


Python
3.8 ou superior


Flask
2.0 ou superior


OpenCV
opencv-python


Requests
requests


Navegador
Qualquer navegador moderno (Chrome, Firefox, Safari, etc.)


📦 Estrutura do Projeto
.
├── Arduino/
│   └── motion_detector.ino    # Código para TTGO T-Camera
├── server/
│   ├── server.py             # Servidor Flask (Python)
│   ├── static/
│   │   └── index.html        # Dashboard web responsivo
│   ├── photos/               # Pasta para fotos capturadas
│   └── logs.json             # Arquivo de logs persistentes
├── LICENSE                   # Licença MIT
└── README.md                 # Documentação

🚀 Configuração
1. Configurar o Ambiente

Instalar Arduino IDE:

Baixe e instale o Arduino IDE.
Adicione suporte para ESP32: File > Preferences > Additional Boards Manager URLs e insira:https://raw.githubusercontent.com/espressif/arduino-esp32/master/package_esp32_index.json


Instale o pacote esp32 em Tools > Board > Boards Manager.


Instalar Bibliotecas Arduino:

No Arduino IDE, vá para Sketch > Include Library > Manage Libraries e instale:
ArduinoJson (by Benoit Blanchon)
Adafruit GFX Library
Adafruit SSD1306




Instalar Dependências Python:
pip3 install flask opencv-python requests


Configurar Fuso Horário:

Certifique-se de que o Mac está no fuso horário de São Paulo:systemsetup -gettimezone


Se necessário, ajuste:sudo systemsetup -settimezone America/Sao_Paulo







2. Configurar o Hardware

TTGO T-Camera:
Conecte o TTGO T-Camera ao computador via USB.
O sensor PIR integrado está no pino GPIO33 (não requer conexões adicionais).


Webcam Logitech C920:
Conecte a webcam ao Mac via USB.
Verifique se está reconhecida:system_profiler SPCameraDataType


Conceda permissões de câmera ao Terminal em Preferências do Sistema > Segurança e Privacidade > Câmera.



3. Configurar o Código Arduino

Abra o arquivo Arduino/motion_detector.ino no Arduino IDE.
Atualize as credenciais de Wi-Fi:const char* sta_ssid = "SUA_REDE_WIFI";
const char* sta_password = "SUA_SENHA_WIFI";


Verifique as configurações do servidor e webhook:const char* macServer = "192.168.1.4";
const int macServerPort = 8080;
const char* discordWebhook = "https://discord.com/api/webhooks/1385051924616708239/Ff4_olAbQ2k9NLy6_Euah8SUIxIhJzBov9HFoxvzgjvjpwWmOF0KSUun8xBIL40MbNSd";


Faça upload do código:
Selecione ESP32 Wrover Module em Tools > Board.
Conecte o TTGO e clique em Upload.
Abra o Serial Monitor (115200 baud) para verificar a conexão Wi-Fi e o IP do Access Point (192.168.4.1).



4. Configurar o Servidor Flask

Crie a estrutura de diretórios:mkdir -p server/static server/photos


Salve server.py na pasta server/.
Salve index.html na pasta server/static/.
Execute o servidor:cd server
python3 server.py


O servidor estará disponível em http://192.168.1.4:8080.



5. Acessar o Dashboard

Na Rede Local:
Acesse http://192.168.1.4:8080 em um navegador para ver o dashboard com logs, fotos e gráficos.


Via Access Point:
Conecte um dispositivo ao Wi-Fi Takeda (senha: vasscl62).
Acesse http://192.168.4.1, que redireciona para o dashboard do servidor.


Acesso Externo (Opcional):
Use Ngrok para expor o servidor:ngrok http 8080


Acesse a URL pública fornecida pelo Ngrok (ex.: https://abc123.ngrok.io).





6. Testar o Sistema

Acionar o Sensor PIR:
Movimente-se na frente do sensor PIR do TTGO.
Verifique no Serial Monitor:🚨 MOVIMENTO DETECTADO!
✅ Sinal de movimento enviado ao Mac!
✅ Notificação enviada ao Discord!




Verificar Logs no Terminal:
No servidor Flask, veja:🚨 Movimento detectado: [timestamp]
📸 Inicializando captura de foto...
✅ Foto salva: ./photos/photo_[data].jpg
📤 Enviando mensagem para o Discord
✅ Foto enviada ao Discord!




Verificar o Discord:
No canal configurado, veja:
Mensagem: [@everyone] Movimento detectado!
Foto com: ⏰ Horário: DD/MM/YYYY às HH:MM:ss




Verificar o Dashboard:
Acesse http://192.168.1.4:8080.
Confirme que:
Gráficos estão lado a lado (telas >800px).
Logs estão ordenados do mais recente ao mais antigo.
Timestamps estão no formato DD/MM/YYYY às HH:MM:ss.
Fotos correspondem aos timestamps.





🐛 Solução de Problemas

Webcam Não Funciona:

Teste a Logitech C920:import cv2
cap = cv2.VideoCapture(0, cv2.CAP_AVFOUNDATION)
ret, frame = cap.read()
if ret:
    cv2.imwrite("test.jpg", frame)
cap.release()


Verifique permissões em Preferências do Sistema > Segurança e Privacidade > Câmera.
Ajuste CAMERA_INDEX em server.py se necessário.


ESP32 Não Conecta ao Servidor:

Verifique o firewall:sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add-port 8080/tcp


Confirme que o Mac e o ESP32 estão na mesma rede Wi-Fi.
Teste o endpoint:curl -X POST http://192.168.1.4:8080/motion -d '{"timestamp":"test"}' -H "Content-Type: application/json"




Fotos Não Aparecem no Dashboard:

Verifique a pasta photos/ e o endpoint:curl http://192.168.1.4:8080/photos/photo_19_06_2025_às_15-12-00.jpg


Confirme que logs.json contém o campo photo.


Timestamp Incorreto:

Verifique o fuso horário do Mac:systemsetup -gettimezone


Ajuste para America/Sao_Paulo se necessário:sudo systemsetup -settimezone America/Sao_Paulo





🤝 Créditos e Referências

Inspirado no projeto: TTGO Security Camera with PIR Motion Sensor por RobotZero.
Bibliotecas utilizadas: Flask, OpenCV, Chart.js, ArduinoJson, Adafruit GFX, Adafruit SSD1306.

📄 Licença
MIT License - Consulte o arquivo LICENSE para detalhes.

Desenvolvido com 💻 e ☕ por [hugotakeda]
