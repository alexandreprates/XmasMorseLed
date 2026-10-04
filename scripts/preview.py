#!/usr/bin/env python3
"""Serve the embedded UI against the native C++ API, without emulating hardware."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import tempfile
import threading

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8080)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="xmas-preview-") as build:
        binary = str(Path(build) / "api")
        subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-Iinclude",
                        "src/MorseTable.cpp", "src/Settings.cpp", "src/Configuration.cpp",
                        "src/ConfigApi.cpp", "tests/host_api.cpp", "-o", binary], cwd=ROOT, check=True)
        worker = subprocess.Popen([binary], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
        lock = threading.Lock()

        class Handler(BaseHTTPRequestHandler):
            def reply(self, status, body, content_type="application/json; charset=utf-8"):
                if isinstance(body, str):
                    body = body.encode("utf-8")
                self.send_response(status)
                self.send_header("Content-Type", content_type)
                self.send_header("Cache-Control", "no-store")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

            def api(self, command):
                with lock:
                    worker.stdin.write(command + "\n")
                    worker.stdin.flush()
                    status, body = worker.stdout.readline().rstrip("\n").split("\t", 1)
                self.reply(int(status), body)

            def do_GET(self):
                if self.path == "/":
                    self.reply(200, (ROOT / "web/index.html").read_bytes(), "text/html; charset=utf-8")
                elif self.path == "/api/config":
                    self.api("GET")
                else:
                    self.reply(404, '{"error":"not_found"}')

            def do_POST(self):
                if self.path != "/api/config":
                    self.reply(404, '{"error":"not_found"}')
                    return
                try:
                    size = int(self.headers.get("Content-Length", "0"))
                except ValueError:
                    size = 0
                if size > 1024:
                    self.reply(413, '{"error":"body_too_large"}')
                elif size <= 0 or not self.headers.get("Content-Type", "").startswith("application/x-www-form-urlencoded"):
                    self.reply(400, '{"error":"invalid_form"}')
                else:
                    self.api("POST " + self.rfile.read(size).hex())

            def log_message(self, *_args):
                pass

        server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
        print(f"Preview: http://127.0.0.1:{args.port} (native API, temporary storage, no hardware)", flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass
        finally:
            server.server_close()
            worker.stdin.close()
            worker.wait(timeout=5)


if __name__ == "__main__":
    main()
