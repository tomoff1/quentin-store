"""Verifie le script livre et le flux TCP ELF, sans contacter une PS5."""
import argparse, hashlib, pathlib, socket, subprocess, tempfile, threading, zipfile
p=argparse.ArgumentParser();p.add_argument('--distribution',required=True);p.add_argument('--powershell',default='pwsh');p.add_argument('--work',default='build/sender-tests');a=p.parse_args()
work=pathlib.Path(a.work).resolve();work.mkdir(parents=True,exist_ok=True);root=pathlib.Path(tempfile.mkdtemp(dir=work))
with zipfile.ZipFile(a.distribution) as z:
    for name in z.namelist():
        if root not in (root/name).resolve().parents:raise RuntimeError('Archive dangereuse')
    z.extractall(root)
expected=(root/'quentin-store.elf').read_bytes();chunks=[]
with socket.socket() as server:
    server.bind(('127.0.0.1',0));server.listen(1);server.settimeout(20)
    def receive():
        conn,_=server.accept()
        with conn:
            conn.settimeout(20)
            while True:
                b=conn.recv(65536)
                if not b:break
                chunks.append(b)
    t=threading.Thread(target=receive);t.start()
    subprocess.run([a.powershell,'-NoProfile','-File',str(root/'scripts/send-elf.ps1'),'-ConsoleIP','127.0.0.1','-Port',str(server.getsockname()[1])],check=True,timeout=30)
    t.join(25)
    actual=b''.join(chunks)
    assert actual==expected and actual.startswith(b'\x7fELF'), 'ELF envoye incorrect'
    print('Archive extraite et script PowerShell executes: '+str(len(actual))+' octets ELF identiques recus localement.')
