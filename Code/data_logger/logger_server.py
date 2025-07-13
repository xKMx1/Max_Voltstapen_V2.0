from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import csv
import os
from datetime import datetime
import threading

HOST = "0.0.0.0"
PORT = 8080
CSV_FILE = "esp32_logs.csv"

# Lock dla bezpiecznego zapisu do pliku
file_lock = threading.Lock()

class ESP32LogHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        raw_post_data = self.rfile.read(content_length)
        
        try:
            data = json.loads(raw_post_data)
            print(f"Received data: {data}")

            # Zapisz dane do pliku CSV z lockiem
            with file_lock:
                # Sprawdź czy plik istnieje, jeśli nie - utwórz z nagłówkami
                file_exists = os.path.exists(CSV_FILE)
                
                with open(CSV_FILE, mode='a', newline='', encoding='utf-8') as file:
                    writer = csv.writer(file)
                    
                    # Jeśli plik nie istnieje, dodaj nagłówki
                    if not file_exists:
                        writer.writerow([
                            "datetime",
                            "timestamp",
                            "line_error", 
                            "duty_left",
                            "duty_right",
                            "enc_l",
                            "enc_r"
                        ])
                        print(f"Created new CSV file: {CSV_FILE}")
                    
                    # Zapisz dane
                    writer.writerow([
                        datetime.now().isoformat(),
                        data.get("timestamp"),
                        data.get("line_error"),
                        data.get("duty_left"),
                        data.get("duty_right"),
                        data.get("enc_l"),
                        data.get("enc_r")
                    ])

            # Odpowiedź OK
            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(b"OK")

        except json.JSONDecodeError as e:
            print(f"JSON decode error: {e}")
            self.send_response(400)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(b"Invalid JSON")
            
        except Exception as e:
            print(f"Error processing POST data: {e}")
            self.send_response(500)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(b"Internal Server Error")

    def do_GET(self):
        # Dodaj prostą stronę statusu
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        
        # Sprawdź czy plik CSV istnieje i ile ma wpisów
        if os.path.exists(CSV_FILE):
            with open(CSV_FILE, 'r') as f:
                line_count = sum(1 for line in f) - 1  # -1 dla nagłówka
        else:
            line_count = 0
            
        response = f"""
        <!DOCTYPE html>
        <html>
        <head><title>ESP32 Logger Status</title></head>
        <body>
            <h1>ESP32 Logger Server</h1>
            <p>Server is running on {HOST}:{PORT}</p>
            <p>CSV file: {CSV_FILE}</p>
            <p>Logged entries: {line_count}</p>
            <p>Last update: {datetime.now().isoformat()}</p>
        </body>
        </html>
        """
        self.wfile.write(response.encode('utf-8'))

    def log_message(self, format, *args):
        # Wyświetl timestamp w logach serwera
        print(f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] {format % args}")

def run_server():
    # Utwórz plik CSV z nagłówkami jeśli nie istnieje
    if not os.path.exists(CSV_FILE):
        with open(CSV_FILE, mode='w', newline='', encoding='utf-8') as file:
            writer = csv.writer(file)
            writer.writerow([
                "datetime",
                "timestamp", 
                "line_error",
                "duty_left",
                "duty_right",
                "enc_l",
                "enc_r"
            ])
        print(f"Created CSV file with headers: {CSV_FILE}")
    
    server = HTTPServer((HOST, PORT), ESP32LogHandler)
    print(f"ESP32 Logger Server listening on http://{HOST}:{PORT}")
    print(f"Logging to: {CSV_FILE}")
    print(f"Access http://localhost:{PORT} for status page")
    print("Press Ctrl+C to stop the server")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped.")
        server.server_close()

if __name__ == "__main__":
    run_server()