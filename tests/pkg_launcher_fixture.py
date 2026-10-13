import pathlib,socket,subprocess,threading,hashlib
binary=pathlib.Path('work/pkg-launcher-host.exe').resolve()
elf=pathlib.Path('work/pkg-extracted/uroot/quentin-store.elf').resolve()
expected=pathlib.Path('outputs/QUENTIN-STORE/dist/quentin-store.elf').read_bytes()
assert elf.read_bytes()==expected
server=socket.socket();server.bind(('127.0.0.1',0));port=server.getsockname()[1];server.listen(1);result=[]
def receive():
 conn,_=server.accept();conn.settimeout(15);data=bytearray()
 with conn:
  while True:
   b=conn.recv(8192)
   if not b:break
   data.extend(b)
 result.append(bytes(data))
t=threading.Thread(target=receive);t.start()
subprocess.run([str(binary),str(elf),str(port)],check=True,timeout=20);t.join(20);server.close()
assert result==[expected]
failed=subprocess.run([str(binary),str(elf),str(port)],timeout=20)
assert failed.returncode==5
print('PKG extracted ELF matches release; actual launcher send loop transferred '+str(len(expected))+' identical bytes; unavailable loader handled.')
