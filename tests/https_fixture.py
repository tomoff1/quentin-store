"""Serveur TLS local et essais du moteur reel, sans Internet."""
import argparse, http.server, pathlib, ssl, subprocess, threading, tempfile
PAYLOAD=b'QUENTIN42\n'*10000
class Handler(http.server.BaseHTTPRequestHandler):
    ranges=[]; dropped=False
    def log_message(self,*a):pass
    def do_GET(self):
        route=self.path
        if route=='/404':self.send_error(404);return
        payload=b'{"schema":1,"items":[]}' if route=='/json' else b'X'*(1024*1024+1) if route=='/large-json' else PAYLOAD
        if route=='/corrupt':payload=b'X'*len(PAYLOAD)
        if route=='/short':payload=PAYLOAD[:999]
        if route=='/oversize':payload=PAYLOAD+b'EXTRA'
        start=0
        if self.headers.get('Range') and route!='/ignore':
            start=int(self.headers['Range'].split('=')[1].split('-')[0]);self.ranges.append((route,start));self.send_response(206)
            wrong=start+1 if route=='/wrong-range' else start
            self.send_header('Content-Range',f'bytes {wrong}-{len(payload)-1}/{len(payload)}')
        else:self.send_response(200)
        self.send_header('Content-Length',str(len(payload)-start));self.end_headers()
        try:
            if route=='/drop' and not Handler.dropped:
                Handler.dropped=True;self.wfile.write(payload[start:start+len(payload)//2]);self.wfile.flush();self.close_connection=True;return
            self.wfile.write(payload[start:])
        except (BrokenPipeError,ConnectionResetError,ssl.SSLError):pass
def main():
    p=argparse.ArgumentParser();p.add_argument('--binary',required=True);p.add_argument('--openssl',default='openssl');p.add_argument('--work',default='build/tls-tests');a=p.parse_args()
    root=pathlib.Path(a.work).resolve();root.mkdir(parents=True,exist_ok=True)
    cert=root/'cert.pem';key=root/'key.pem'
    subprocess.run([a.openssl,'req','-x509','-newkey','rsa:2048','-nodes','-keyout',str(key),'-out',str(cert),'-days','1','-subj','/CN=localhost','-addext','subjectAltName=DNS:localhost'],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);tls=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);tls.load_cert_chain(cert,key);server.socket=tls.wrap_socket(server.socket,server_side=True)
    t=threading.Thread(target=server.serve_forever,daemon=True);t.start()
    try:
        download_dir=tempfile.mkdtemp(prefix='downloads-',dir=root)
        subprocess.run([str(pathlib.Path(a.binary).resolve()),f'https://localhost:{server.server_port}',str(cert),download_dir],check=True)
        assert ('/asset',1234) in Handler.ranges and ('/drop',len(PAYLOAD)//2) in Handler.ranges,Handler.ranges
        print('Reprise HTTP Range effectivement observee; interruption et redemarrage verifies.')
    finally:server.shutdown();server.server_close()
if __name__=='__main__':main()
