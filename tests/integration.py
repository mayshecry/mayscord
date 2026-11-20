import http.server
import json
import pathlib
import shlex
import socketserver
import subprocess
import tempfile
import threading

root = pathlib.Path(__file__).resolve().parent.parent
requests = []

class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def handle_request(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", "0")))
        requests.append((self.command, self.path, dict(self.headers), body))
        if self.path.endswith("/typing") and sum(p.endswith("/typing") for _, p, _, _ in requests) == 1:
            self.send_response(429)
            self.send_header("Retry-After", "0.01")
            self.end_headers()
            self.wfile.write(b'{}')
            return
        status = 204 if self.command in ("PUT", "DELETE") or self.path.endswith("/typing") else 200
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        if status != 204:
            self.wfile.write(b'{}')

    do_GET = handle_request
    do_POST = handle_request
    do_PUT = handle_request
    do_DELETE = handle_request

with socketserver.TCPServer(("127.0.0.1", 0), Handler) as server, tempfile.TemporaryDirectory() as temp:
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    temp = pathlib.Path(temp)
    port = server.server_address[1]
    header = (root / "src/internal/internal.h").read_text()
    header = header.replace('"https://discord.com/api/v10"', f'"http://127.0.0.1:{port}/api/v10"')
    (temp / "internal.h").write_text(header)
    sources = sorted((root / "src").rglob("*.c"))
    (temp / "upload.txt").write_text("upload payload")
    flags = shlex.split(subprocess.check_output(["pkg-config", "--cflags", "--libs", "libcurl"], text=True))
    subprocess.run(["cc", "-std=c99", "-I", str(root / "include"), "-I", str(temp), *map(str, sources), str(root / "tests/integration.c"), *flags, "-o", str(temp / "integration")], check=True)
    subprocess.run([str(temp / "integration"), str(temp / "upload.txt")], check=True, timeout=20)
    server.shutdown()
    thread.join()

assert len(requests) == 12, requests
assert all(h.get("Authorization") == "Bot test.token" for _, _, h, _ in requests)
assert json.loads(requests[0][3]) == {"content": 'Hello "world"', "allowed_mentions": {"parse": []}}
assert b'upload payload' in requests[1][3] and b'name="files[0]"' in requests[1][3]
assert b'name="payload_json"' in requests[1][3]
assert requests[2][1].endswith('/typing') and requests[3][1].endswith('/typing')
assert requests[4][0:2] == ('PUT', '/api/v10/channels/123/messages/456/reactions/hello%3A789/@me')
assert requests[5][0] == 'DELETE' and requests[5][1] == requests[4][1]
assert requests[6][0:2] == ('PUT', '/api/v10/guilds/1/members/2/roles/3')
assert requests[7][0:2] == ('DELETE', '/api/v10/guilds/1/members/2/roles/3')
assert requests[8][1] == '/api/v10/channels/123/messages?limit=10&before=456'
assert json.loads(requests[9][3]) == {'recipient_id': '2'}
assert json.loads(requests[10][3])['embeds'][0]['color'] == 0xff00ff
assert json.loads(requests[11][3])['data']['flags'] == 64
print('integration ok')
