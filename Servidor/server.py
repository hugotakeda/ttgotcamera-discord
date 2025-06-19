import os
import cv2
import flask
import requests
import json
from datetime import datetime
import time
import glob
import pytz

# Desativar solicitação automática de autorização do OpenCV
os.environ["OPENCV_AVFOUNDATION_SKIP_AUTH"] = "1"

app = flask.Flask(__name__, static_folder="static")

# Configurações
CAMERA_INDEX = 0
SAVE_DIR = "./photos"
LOG_FILE = "./logs.json"
DISCORD_WEBHOOK = "https://discord.com/api/webhooks/1385051924616708239/Ff4_olAbQ2k9NLy6_Euah8SUIxIhJzBov9HFoxvzgjvjpwWmOF0KSUun8xBIL40MbNSd"
CLEANUP_OLDER_THAN_DAYS = 7
SAO_PAULO_TZ = pytz.timezone("America/Sao_Paulo")

# Criar diretórios
if not os.path.exists(SAVE_DIR):
    os.makedirs(SAVE_DIR)
    print(f"✅ Diretório {SAVE_DIR} criado")

def load_logs():
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r") as f:
            try:
                return json.load(f)
            except:
                return []
    return []

def save_log(timestamp, file_path):
    logs = load_logs()
    logs.append({"timestamp": timestamp, "photo": file_path})
    with open(LOG_FILE, "w") as f:
        json.dump(logs, f, indent=2)

def clear_logs():
    if os.path.exists(LOG_FILE):
        os.remove(LOG_FILE)
    for f in glob.glob(f"{SAVE_DIR}/*.jpg"):
        os.remove(f)
        print(f"🗑️ Foto removida: {f}")

def cleanup_old_photos():
    now = time.time()
    for f in glob.glob(f"{SAVE_DIR}/*.jpg"):
        if os.path.getctime(f) < now - CLEANUP_OLDER_THAN_DAYS * 86400:
            os.remove(f)
            print(f"🗑️ Foto antiga removida: {f}")

def capture_photo():
    print("📸 Inicializando captura de foto...")
    cap = cv2.VideoCapture(CAMERA_INDEX, cv2.CAP_AVFOUNDATION)
    if not cap.isOpened():
        print("❌ Erro ao abrir a webcam")
        return None, None

    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
    for _ in range(5):
        cap.grab()

    ret, frame = cap.read()
    if not ret:
        print("❌ Erro ao capturar imagem")
        cap.release()
        return None, None

    timestamp = datetime.now(SAO_PAULO_TZ).strftime("%d/%m/%Y às %H:%M:%S")
    file_path = os.path.join(SAVE_DIR, f"photo_{timestamp.replace('/', '_').replace(':', '-').replace(' às ', '_')}.jpg")
    cv2.imwrite(file_path, frame, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
    print(f"✅ Foto salva: {file_path}")
    cap.release()
    return file_path, timestamp

def send_to_discord(file_path, timestamp):
    print(f"📤 Enviando mensagem para o Discord")
    try:
        # Enviar mensagem de alerta
        data = {
            "content": "[@everyone] Movimento detectado!",
            "username": "TTGO Motion Detector"
        }
        response = requests.post(DISCORD_WEBHOOK, json=data)
        if response.status_code not in (200, 204):
            print(f"❌ Falha ao enviar alerta ao Discord: {response.status_code}")

        # Enviar foto com timestamp
        if file_path:
            with open(file_path, "rb") as f:
                files = {"file": (os.path.basename(file_path), f, "image/jpeg")}
                data = {
                    "content": f"⏰ Horário: {timestamp}",
                    "username": "TTGO Motion Detector"
                }
                response = requests.post(DISCORD_WEBHOOK, data=data, files=files)
                if response.status_code in (200, 204):
                    print("✅ Foto enviada ao Discord!")
                else:
                    print(f"❌ Falha ao enviar foto ao Discord: {response.status_code}")
    except Exception as e:
        print(f"❌ Erro ao enviar para o Discord: {str(e)}")

@app.route("/", methods=["GET"])
def handle_root():
    return flask.send_file("static/index.html")

@app.route("/logs", methods=["GET"])
def get_logs():
    logs = load_logs()
    return flask.jsonify(logs)

@app.route("/photos/<path:filename>")
def serve_photo(filename):
    return flask.send_from_directory(SAVE_DIR, filename)

@app.route("/motion", methods=["POST"])
def handle_motion():
    data = flask.request.get_json()
    timestamp = data.get("timestamp", "Desconhecido")
    print(f"🚨 Movimento detectado: {timestamp}")

    cleanup_old_photos()
    file_path, capture_timestamp = capture_photo()
    if file_path:
        save_log(capture_timestamp, os.path.basename(file_path))
        send_to_discord(file_path, capture_timestamp)

    return {"status": "success", "timestamp": timestamp}, 200

@app.route("/clear_logs", methods=["POST"])
def clear_logs_endpoint():
    clear_logs()
    return {"status": "logs cleared"}, 200

if __name__ == "__main__":
    print("🚀 Iniciando servidor Flask na porta 8080...")
    app.run(host="0.0.0.0", port=8080)
