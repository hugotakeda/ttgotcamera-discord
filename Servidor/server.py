import os
import cv2
import flask
import requests
import json
from datetime import datetime
import time
import glob
import pytz
import platform
import subprocess
import sys
import re

# Desativar solicitação automática de autorização do OpenCV
os.environ["OPENCV_AVFOUNDATION_SKIP_AUTH"] = "1"

app = flask.Flask(__name__)

# Configurações
SAVE_DIR = "./photos"
LOG_FILE = "./logs.json"
DISCORD_WEBHOOK = "https://discord.com/api/webhooks/1385346002709381343/G1FI6RC7OHd8sJVJCIEqabSqtjB-OTtSnP4UO3vW3jaNach0LmQptjqU-IizDGLZfVrP"
CLEANUP_OLDER_THAN_DAYS = 7
SAO_PAULO_TZ = pytz.timezone("America/Sao_Paulo")

# Variáveis globais para câmera
EXTERNAL_CAMERA_INDEX = None
CAMERA_BACKEND = None
CAMERA_INFO = {}

# Criar diretórios necessários
directories = [SAVE_DIR]
for directory in directories:
    if not os.path.exists(directory):
        os.makedirs(directory)
        print(f"✅ Diretório {directory} criado")

def find_logitech_c920():
    """Encontra especificamente a Logitech C920"""
    print("🔍 Procurando especificamente pela Logitech C920...")
    
    if platform.system() == "Darwin":  # macOS
        try:
            # Usar system_profiler para encontrar câmeras USB
            result = subprocess.run(['system_profiler', 'SPUSBDataType'], 
                                  capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                output = result.stdout
                print("📋 Dispositivos USB encontrados:")
                
                # Procurar por Logitech
                logitech_devices = []
                lines = output.split('\n')
                for i, line in enumerate(lines):
                    if 'logitech' in line.lower() or 'c920' in line.lower():
                        logitech_devices.append(line.strip())
                        print(f"  🎥 Encontrado: {line.strip()}")
                
                if logitech_devices:
                    print("✅ Logitech C920 detectada no sistema!")
                else:
                    print("⚠️ Logitech C920 não encontrada nos dispositivos USB")
                    
        except Exception as e:
            print(f"⚠️ Erro ao verificar dispositivos USB: {e}")
    
    elif platform.system() == "Linux":
        try:
            # Verificar dispositivos v4l2
            result = subprocess.run(['v4l2-ctl', '--list-devices'], 
                                  capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                output = result.stdout
                print("📋 Dispositivos V4L2:")
                print(output)
                
                if 'logitech' in output.lower() or 'c920' in output.lower():
                    print("✅ Logitech C920 detectada!")
                else:
                    print("⚠️ Logitech C920 não encontrada")
        except Exception as e:
            print(f"⚠️ v4l2-ctl não disponível: {e}")

def detect_cameras_advanced():
    """Detecta câmeras com foco na Logitech C920"""
    print("🔍 Detectando câmeras com foco na Logitech C920...")
    
    # Primeiro, procurar especificamente pela C920
    find_logitech_c920()
    
    available_cameras = []
    
    # Testar diferentes backends
    backends = []
    if platform.system() == "Darwin":  # macOS
        backends = [cv2.CAP_AVFOUNDATION]
    elif platform.system() == "Windows":
        backends = [cv2.CAP_DSHOW, cv2.CAP_MSMF]
    else:  # Linux
        backends = [cv2.CAP_V4L2]
    
    # Testar índices de câmera de 0 a 10
    for backend in backends:
        print(f"🧪 Testando backend: {backend}")
        for i in range(11):  # 0 a 10
            try:
                print(f"  📹 Testando câmera índice {i}...")
                cap = cv2.VideoCapture(i, backend)
                
                if cap.isOpened():
                    # Configurar resolução para teste
                    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
                    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
                    
                    # Tentar capturar frames
                    success_count = 0
                    test_frames = 3
                    
                    for attempt in range(test_frames):
                        ret, frame = cap.read()
                        if ret and frame is not None and frame.size > 0:
                            success_count += 1
                        time.sleep(0.2)
                    
                    if success_count >= 2:  # Pelo menos 2/3 capturas bem-sucedidas
                        # Obter informações da câmera
                        width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
                        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
                        fps = int(cap.get(cv2.CAP_PROP_FPS))
                        
                        # Tentar identificar se é uma C920 baseado na resolução
                        is_c920_candidate = False
                        if width >= 1920 and height >= 1080:  # C920 suporta Full HD
                            is_c920_candidate = True
                            print(f"  🎯 POSSÍVEL C920 DETECTADA! Índice {i}")
                        
                        camera_info = {
                            'index': i,
                            'backend': backend,
                            'width': width,
                            'height': height,
                            'fps': fps,
                            'success_rate': success_count / test_frames,
                            'is_c920_candidate': is_c920_candidate,
                            'priority': 100 if is_c920_candidate else (50 if i > 0 else 10)
                        }
                        
                        available_cameras.append(camera_info)
                        print(f"  ✅ Câmera {i}: {width}x{height} @ {fps}fps "
                              f"(Taxa: {success_count}/{test_frames}) "
                              f"{'🎯 POSSÍVEL C920!' if is_c920_candidate else ''}")
                    else:
                        print(f"  ❌ Câmera {i}: Taxa de sucesso baixa ({success_count}/{test_frames})")
                    
                    cap.release()
                else:
                    print(f"  ❌ Câmera {i}: Não foi possível abrir")
                    
            except Exception as e:
                print(f"  ❌ Câmera {i}: Erro - {e}")
                continue
    
    if not available_cameras:
        print("❌ Nenhuma câmera funcional detectada!")
        return None, None, {}
    
    print(f"\n✅ {len(available_cameras)} câmera(s) funcional(is) detectada(s)")
    
    # Mostrar todas as câmeras encontradas
    print("\n📋 Resumo das câmeras:")
    for cam in available_cameras:
        status = "🎯 POSSÍVEL C920" if cam['is_c920_candidate'] else ("📹 Externa" if cam['index'] > 0 else "💻 Interna")
        print(f"  Índice {cam['index']}: {cam['width']}x{cam['height']} @ {cam['fps']}fps "
              f"(Backend: {cam['backend']}, Taxa: {cam['success_rate']:.1%}) - {status}")
    
    # Priorizar câmeras candidatas à C920, depois externas, depois internas
    best_camera = max(available_cameras, key=lambda x: (x['priority'], x['width'] * x['height'], x['success_rate']))
    
    camera_type = "🎯 LOGITECH C920 (CANDIDATA)" if best_camera['is_c920_candidate'] else \
                  ("📹 CÂMERA EXTERNA" if best_camera['index'] > 0 else "💻 CÂMERA INTERNA")
    
    print(f"\n🎯 CÂMERA SELECIONADA: {camera_type}")
    print(f"   Índice: {best_camera['index']}")
    print(f"   Resolução: {best_camera['width']}x{best_camera['height']}")
    print(f"   Backend: {best_camera['backend']}")
    print(f"   Taxa de Sucesso: {best_camera['success_rate']:.1%}")
    
    return best_camera['index'], best_camera['backend'], best_camera

def initialize_camera():
    """Inicializa a câmera com foco na Logitech C920"""
    global EXTERNAL_CAMERA_INDEX, CAMERA_BACKEND, CAMERA_INFO
    
    camera_index, backend, camera_info = detect_cameras_advanced()
    if camera_index is None:
        print("❌ Erro: Nenhuma câmera funcional disponível!")
        return False
    
    EXTERNAL_CAMERA_INDEX = camera_index
    CAMERA_BACKEND = backend
    CAMERA_INFO = camera_info
    
    # Teste final com configurações otimizadas para C920
    print(f"\n🧪 Teste final da câmera selecionada...")
    cap = cv2.VideoCapture(EXTERNAL_CAMERA_INDEX, CAMERA_BACKEND)
    
    if not cap.isOpened():
        print(f"❌ Erro ao abrir câmera índice {EXTERNAL_CAMERA_INDEX}")
        return False
    
    # Configurações específicas para C920
    if camera_info.get('is_c920_candidate'):
        print("🎯 Aplicando configurações otimizadas para Logitech C920...")
        # C920 suporta até 1920x1080 @ 30fps
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
        cap.set(cv2.CAP_PROP_FPS, 30)
    else:
        print("📹 Aplicando configurações padrão...")
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
        cap.set(cv2.CAP_PROP_FPS, 30)
    
    # Configurações adicionais
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Reduzir buffer
    cap.set(cv2.CAP_PROP_AUTOFOCUS, 1)   # Autofoco
    
    # Teste de captura
    print("📸 Realizando teste final de captura...")
    success_count = 0
    for i in range(5):
        ret, frame = cap.read()
        if ret and frame is not None and frame.size > 0:
            success_count += 1
        time.sleep(0.1)
    
    actual_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    actual_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    actual_fps = int(cap.get(cv2.CAP_PROP_FPS))
    
    cap.release()
    
    if success_count < 4:
        print(f"❌ Taxa de sucesso muito baixa: {success_count}/5")
        return False
    
    print(f"\n✅ CÂMERA INICIALIZADA COM SUCESSO!")
    print(f"📐 Resolução Final: {actual_width}x{actual_height}")
    print(f"🎬 FPS: {actual_fps}")
    print(f"📊 Taxa de Sucesso: {success_count}/5")
    print(f"🎯 Tipo: {'Logitech C920 (Candidata)' if camera_info.get('is_c920_candidate') else 'Câmera Padrão'}")
    
    return True

def load_logs():
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r", encoding='utf-8') as f:
            try:
                return json.load(f)
            except:
                return []
    return []

def save_log(timestamp, file_path, device_info=None):
    logs = load_logs()
    log_entry = {
        "timestamp": timestamp, 
        "photo": file_path,
        "device_info": device_info or "TTGO T-Camera ESP32"
    }
    logs.append(log_entry)
    with open(LOG_FILE, "w", encoding='utf-8') as f:
        json.dump(logs, f, indent=2, ensure_ascii=False)

def clear_logs():
    if os.path.exists(LOG_FILE):
        os.remove(LOG_FILE)
        print("🗑️ Arquivo de logs removido")
    
    photo_count = 0
    for f in glob.glob(f"{SAVE_DIR}/*.jpg"):
        os.remove(f)
        photo_count += 1
    
    print(f"🗑️ {photo_count} foto(s) removida(s)")

def cleanup_old_photos():
    now = time.time()
    removed_count = 0
    
    for f in glob.glob(f"{SAVE_DIR}/*.jpg"):
        if os.path.getctime(f) < now - CLEANUP_OLDER_THAN_DAYS * 86400:
            os.remove(f)
            removed_count += 1
    
    if removed_count > 0:
        print(f"🗑️ {removed_count} foto(s) antiga(s) removida(s)")

def capture_photo():
    """Captura foto usando a câmera selecionada (preferencialmente C920)"""
    if EXTERNAL_CAMERA_INDEX is None:
        print("❌ Câmera não inicializada")
        return None, None
    
    camera_type = "Logitech C920 (Candidata)" if CAMERA_INFO.get('is_c920_candidate') else "Câmera Padrão"
    print(f"📸 Capturando foto com {camera_type} (índice {EXTERNAL_CAMERA_INDEX})...")
    
    # Múltiplas tentativas
    for attempt in range(3):
        print(f"🔄 Tentativa {attempt + 1}/3...")
        
        cap = cv2.VideoCapture(EXTERNAL_CAMERA_INDEX, CAMERA_BACKEND)
        
        if not cap.isOpened():
            print(f"❌ Tentativa {attempt + 1}: Erro ao abrir câmera")
            continue

        # Configurar câmera baseado no tipo
        if CAMERA_INFO.get('is_c920_candidate'):
            # Configurações otimizadas para C920
            cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
            cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
            cap.set(cv2.CAP_PROP_FPS, 30)
        else:
            cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
            cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
            cap.set(cv2.CAP_PROP_FPS, 30)
        
        cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        cap.set(cv2.CAP_PROP_AUTOFOCUS, 1)
        
        # Aguardar estabilização
        print("⏳ Aguardando estabilização...")
        for i in range(10):
            ret = cap.grab()
            if not ret:
                print(f"❌ Tentativa {attempt + 1}: Erro no frame {i+1}")
                break
            time.sleep(0.1)
        else:
            # Capturar frame final
            ret, frame = cap.read()
            cap.release()
            
            if ret and frame is not None and frame.size > 0:
                # Gerar timestamp e nome do arquivo
                timestamp = datetime.now(SAO_PAULO_TZ).strftime("%d/%m/%Y às %H:%M:%S")
                safe_timestamp = timestamp.replace('/', '_').replace(':', '-').replace(' às ', '_')
                file_path = os.path.join(SAVE_DIR, f"motion_{safe_timestamp}.jpg")
                
                # Salvar com qualidade alta
                encode_params = [int(cv2.IMWRITE_JPEG_QUALITY), 95]
                success = cv2.imwrite(file_path, frame, encode_params)
                
                if success and os.path.exists(file_path):
                    file_size = os.path.getsize(file_path) / 1024  # KB
                    height, width = frame.shape[:2]
                    print(f"✅ Foto capturada com sucesso!")
                    print(f"📁 Arquivo: {os.path.basename(file_path)}")
                    print(f"📐 Resolução: {width}x{height}")
                    print(f"📊 Tamanho: {file_size:.1f} KB")
                    print(f"🎥 Câmera: {camera_type}")
                    return file_path, timestamp
                else:
                    print(f"❌ Tentativa {attempt + 1}: Erro ao salvar foto")
            else:
                print(f"❌ Tentativa {attempt + 1}: Frame inválido")
        
        cap.release()
        time.sleep(0.5)
    
    print("❌ Falha em todas as tentativas de captura")
    return None, None

def send_to_discord(file_path, timestamp, device_info=None):
    """Envia foto e informações para o Discord"""
    print(f"📤 Enviando notificação para o Discord...")
    
    try:
        # Primeira mensagem: Alerta
        alert_data = {
            "content": "🚨 **MOVIMENTO DETECTADO!** 🚨",
            "username": "🎥 TTGO Motion Detector",
            "avatar_url": "https://cdn-icons-png.flaticon.com/512/1001/1001371.png"
        }
        
        response = requests.post(DISCORD_WEBHOOK, json=alert_data, timeout=10)
        if response.status_code not in (200, 204):
            print(f"⚠️ Falha ao enviar alerta: {response.status_code}")

        # Segunda mensagem: Foto com detalhes
        if file_path and os.path.exists(file_path):
            with open(file_path, "rb") as f:
                files = {"file": (os.path.basename(file_path), f, "image/jpeg")}
                
                camera_type = "🎯 Logitech C920" if CAMERA_INFO.get('is_c920_candidate') else "📹 Câmera Externa"
                
                content = f"📸 **FOTO DA DETECÇÃO**\n\n"
                content += f"🕐 **Horário:** {timestamp}\n"
                content += f"📍 **Local:** Sensor PIR (GPIO 33)\n"
                content += f"🎥 **Câmera:** {camera_type} (Índice {EXTERNAL_CAMERA_INDEX})\n"
                content += f"💻 **Servidor:** {platform.system()}\n"
                
                if device_info:
                    content += f"📱 **Dispositivo:** {device_info}\n"
                
                file_size = os.path.getsize(file_path) / 1024
                content += f"📊 **Tamanho:** {file_size:.1f} KB\n"
                
                data = {
                    "content": content,
                    "username": "📸 Camera System",
                    "avatar_url": "https://cdn-icons-png.flaticon.com/512/685/685655.png"
                }
                
                response = requests.post(DISCORD_WEBHOOK, data=data, files=files, timeout=15)
                if response.status_code in (200, 204):
                    print("✅ Foto enviada ao Discord com sucesso!")
                else:
                    print(f"❌ Falha ao enviar foto: {response.status_code}")
        
    except Exception as e:
        print(f"❌ Erro ao enviar para Discord: {str(e)}")

# ROTAS DO FLASK
@app.route("/", methods=["GET"])
def handle_root():
    """Servir página principal"""
    if os.path.exists("index.html"):
        print("📄 Servindo arquivo: index.html")
        return flask.send_file("index.html")
    else:
        print("⚠️ Arquivo index.html não encontrado!")
        return create_basic_html()

def create_basic_html():
    """HTML básico se não encontrar o arquivo principal"""
    return """
    <!DOCTYPE html>
    <html><head><title>TTGO Motion Detector</title></head>
    <body style="font-family:Arial;margin:40px;background:#f5f5f5;">
        <h1>⚠️ Arquivo index.html não encontrado</h1>
        <p>Coloque o arquivo <strong>index.html</strong> na mesma pasta do servidor Python.</p>
        <button onclick="location.reload()">🔄 Recarregar</button>
    </body></html>
    """

@app.route("/logs", methods=["GET"])
def get_logs():
    """Retornar logs em JSON"""
    logs = load_logs()
    print(f"📡 Enviando {len(logs)} logs para o cliente")
    return flask.jsonify(logs)

@app.route("/photos/<path:filename>")
def serve_photo(filename):
    """Servir fotos"""
    return flask.send_from_directory(SAVE_DIR, filename)

@app.route("/motion", methods=["POST"])
def handle_motion():
    """Processar detecção de movimento"""
    try:
        data = flask.request.get_json()
        timestamp = data.get("timestamp", "Horário desconhecido")
        device_info = data.get("device", "TTGO T-Camera ESP32")
        message = data.get("message", "Movimento detectado")
        
        print("🚨" + "="*50)
        print("   MOVIMENTO DETECTADO PELO ARDUINO!")
        print("🚨" + "="*50)
        print(f"📅 Timestamp: {timestamp}")
        print(f"📱 Dispositivo: {device_info}")
        print(f"💬 Mensagem: {message}")
        print("="*52)
        
        # Limpeza de fotos antigas
        cleanup_old_photos()
        
        # Capturar foto
        file_path, capture_timestamp = capture_photo()
        
        if file_path:
            # Salvar log
            save_log(capture_timestamp, os.path.basename(file_path), device_info)
            
            # Enviar para Discord
            send_to_discord(file_path, capture_timestamp, device_info)
            
            print("✅ Processamento concluído com sucesso!")
            
            return {
                "status": "success", 
                "timestamp": timestamp,
                "photo_captured": True,
                "photo_path": os.path.basename(file_path)
            }, 200
        else:
            print("⚠️ Falha na captura da foto")
            return {
                "status": "partial_success", 
                "timestamp": timestamp,
                "photo_captured": False,
                "error": "Falha na captura da foto"
            }, 200
            
    except Exception as e:
        print(f"❌ Erro ao processar movimento: {str(e)}")
        return {"status": "error", "message": str(e)}, 500

@app.route("/clear_logs", methods=["POST"])
def clear_logs_endpoint():
    """Limpar todos os logs e fotos"""
    try:
        clear_logs()
        return {"status": "success", "message": "Logs limpos com sucesso"}, 200
    except Exception as e:
        return {"status": "error", "message": str(e)}, 500

@app.route("/camera/status", methods=["GET"])
def camera_status():
    """Status da câmera"""
    return flask.jsonify({
        "external_camera_index": EXTERNAL_CAMERA_INDEX,
        "camera_backend": CAMERA_BACKEND,
        "camera_initialized": EXTERNAL_CAMERA_INDEX is not None,
        "camera_info": CAMERA_INFO,
        "is_c920_candidate": CAMERA_INFO.get('is_c920_candidate', False),
        "system": platform.system(),
        "opencv_version": cv2.__version__
    })

@app.route("/camera/test", methods=["POST"])
def test_camera():
    """Testar captura da câmera manualmente"""
    try:
        file_path, timestamp = capture_photo()
        if file_path:
            return {
                "status": "success",
                "message": "Foto de teste capturada com sucesso",
                "photo_path": os.path.basename(file_path),
                "timestamp": timestamp,
                "camera_type": "Logitech C920 (Candidata)" if CAMERA_INFO.get('is_c920_candidate') else "Câmera Padrão"
            }, 200
        else:
            return {
                "status": "error",
                "message": "Falha na captura da foto de teste"
            }, 500
    except Exception as e:
        return {"status": "error", "message": str(e)}, 500

if __name__ == "__main__":
    print("🚀 Iniciando TTGO Motion Detection Server...")
    print("🎯 Versão: 2.0 - Otimizado para Logitech C920")
    print("="*60)
    
    # Verificar se o arquivo HTML existe
    if os.path.exists("index.html"):
        print("✅ Arquivo index.html encontrado")
    else:
        print("⚠️ Arquivo index.html não encontrado!")
        print("📝 Coloque o arquivo 'index.html' na mesma pasta do servidor")
    
    print("="*60)
    
    # Inicializar câmera com foco na C920
    if initialize_camera():
        camera_type = "🎯 Logitech C920 (Candidata)" if CAMERA_INFO.get('is_c920_candidate') else "📹 Câmera Padrão"
        print(f"✅ Sistema pronto com {camera_type}!")
    else:
        print("⚠️ Sistema iniciado com limitações na câmera")
        print("💡 Verifique se a Logitech C920 está conectada")
    
    print("="*60)
    print("🌐 Servidor Flask rodando na porta 8080...")
    print("🔗 Acesse: http://localhost:8080")
    print("📱 Aguardando conexões do Arduino...")
    print("🧪 Teste manual: POST /camera/test")
    
    app.run(host="0.0.0.0", port=8080, debug=False)
