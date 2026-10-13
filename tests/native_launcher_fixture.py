"""Run the production native launcher transfer loop on a Windows TCP fixture.

Requires MinGW g++, permits local networking, and never accesses a console.
"""
from pathlib import Path
import argparse,hashlib,json,os,socket,subprocess,threading,zipfile

PROJECT=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--payload',type=Path,required=True)
parser.add_argument('--work',type=Path,default=PROJECT/'build/native-launcher-tests')
parser.add_argument('--cxx',default='g++')
args=parser.parse_args()
work=args.work.resolve();work.mkdir(parents=True,exist_ok=True)
payload=args.payload.resolve(); data=payload.read_bytes()
if data[:4]!=b'\x7fELF':raise SystemExit('Expected a real payload ELF')
include=work/'include';include.mkdir(exist_ok=True)
commit='b8dc4a3fe2e05e826b037bd68824751c7468a6d7'
archive=PROJECT/'third_party/sources'/f'ps5-native-app-boilerplate-{commit}.zip'
if hashlib.sha256(archive.read_bytes()).hexdigest()!='35c10f586bc3a2e80526920f059a851007a8e4c709915a68ee4c1d0b28c47ca8':
    raise SystemExit('Source archive checksum mismatch')
with zipfile.ZipFile(archive) as source:
    (include/'demo_renderer.hpp').write_bytes(source.read(f'ps5-native-app-boilerplate-{commit}/src/demo_renderer.hpp'))
binary=work/'native-launcher-host.exe'
subprocess.run([args.cxx,'-std=c++20','-O2','-DQS_HOST_TEST','-Dopen=launcher_open','-Dread=launcher_read','-Dclose=launcher_close',
    '-I',str(include),str(PROJECT/'native-launcher/main.cpp'),str(PROJECT/'tests/native_launcher_host_shims.cpp'),
    '-lws2_32','-o',str(binary)],check=True)
env=os.environ.copy();env['QS_TEST_PAYLOAD']=str(payload)

def transfer(extra_env=None):
    listener=socket.socket();listener.bind(('127.0.0.1',0));listener.listen(1);listener.settimeout(10)
    port=listener.getsockname()[1]; received=[]; errors=[]
    def receiver():
        try:
            connection,_=listener.accept()
            with connection:
                while True:
                    block=connection.recv(137)
                    if not block:break
                    received.append(block)
        except Exception as error:errors.append(str(error))
    thread=threading.Thread(target=receiver);thread.start()
    run_env=env.copy();run_env['QS_TEST_PORT']=str(port)
    if extra_env:run_env.update(extra_env)
    result=subprocess.run([str(binary)],env=run_env,capture_output=True,text=True,timeout=40)
    thread.join(10);listener.close()
    assert not errors,(errors,result.stderr)
    return result,b''.join(received),port

success,received,closed_port=transfer()
assert success.returncode==0,success.stderr
assert received==data
ordered=['lanceur natif execute','lecture du store embarque','ELF valide','chargeur connecte','ELF transmis']
positions=[success.stderr.index(s) for s in ordered]
assert positions==sorted(positions)
interrupted,received,_=transfer({'QS_TEST_FORCE_SEND_FAIL':'1'})
assert interrupted.returncode==2 and 'ENVOI ELF INTERROMPU' in interrupted.stderr
assert not received and 'ELF transmis' not in interrupted.stderr
env['QS_TEST_PORT']=str(closed_port)
closed=subprocess.run([str(binary)],env=env,capture_output=True,text=True,timeout=15)
assert closed.returncode==2 and '9021 INACCESSIBLE' in closed.stderr
invalid=work/'invalid.payload';invalid.write_bytes(b'not an elf');env['QS_TEST_PAYLOAD']=str(invalid)
bad=subprocess.run([str(binary)],env=env,capture_output=True,text=True,timeout=10)
assert bad.returncode==2 and 'ELF EMBARQUE INVALIDE' in bad.stderr
env['QS_TEST_PAYLOAD']=str(work/'missing.payload')
missing=subprocess.run([str(binary)],env=env,capture_output=True,text=True,timeout=10)
assert missing.returncode==2 and 'INTROUVABLE' in missing.stderr
report={'checks':['actual payload TCP transfer matches bytes','partial sends511B','notifications ordered',
                  'negative send returns failure','closed loader refused','invalid ELF rejected','missing ELF rejected'],
        'payload_bytes':len(data),'payload_sha256':hashlib.sha256(data).hexdigest()}
(work/'native-test-report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,indent=2))
