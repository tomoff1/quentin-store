"""Build the native PS5 launcher folder using the pinned public app converter.

No Sony SDK or proprietary runtime module is required. Creating the PKG
container is a separate packaging step after the native title is compiled.
"""
from pathlib import Path
import argparse, hashlib, json, shutil, subprocess, zipfile, os

PROJECT=Path(__file__).resolve().parent.parent
COMMIT='b8dc4a3fe2e05e826b037bd68824751c7468a6d7'
ARCHIVE_SHA256='35c10f586bc3a2e80526920f059a851007a8e4c709915a68ee4c1d0b28c47ca8'
RAW_RUNTIME_SHA256='8ee6e124993e1af26420cb455890fd002f5d6c7e78883c860ce45734e7d002bb'
SIGNED_RUNTIME_SHA256='e6ff45d16adf687855cc3b33b0c8a4132b6504360b221e0a34c7e99fb3ba0036'

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sdk',type=Path,required=True,help='PS5 Payload SDK 0.44 root')
parser.add_argument('--llvm',type=Path,required=True,help='LLVM 18 bin directory')
parser.add_argument('--upstream',type=Path,help='Optional extracted pinned boilerplate root')
parser.add_argument('--output',type=Path,default=PROJECT/'build/native-ps5')
parser.add_argument('--payload',type=Path,default=PROJECT/'build/ps5/quentin-store.elf')
parser.add_argument('--host-cxx',default='g++')
parser.add_argument('--zlib-include',type=Path,help='Host-compatible zlib.h directory; standard host search if omitted')
parser.add_argument('--zlib-library',type=Path,help='Host zlib archive or DLL; -lz if omitted')
args=parser.parse_args()
output=args.output.resolve();output.mkdir(parents=True,exist_ok=True)
sdk=args.sdk.resolve();llvm=args.llvm.resolve()
payload=args.payload.resolve()
if payload.read_bytes()[:4]!=b'\x7fELF':raise SystemExit('The bundled payload is not an ELF file')

def run(arguments):
    subprocess.run([str(a) for a in arguments],check=True)

if args.upstream:
    upstream=args.upstream.resolve()
else:
    archive=PROJECT/'third_party/sources'/f'ps5-native-app-boilerplate-{COMMIT}.zip'
    if hashlib.sha256(archive.read_bytes()).hexdigest()!=ARCHIVE_SHA256:
        raise SystemExit('Pinned native app source archive checksum mismatch')
    extracted=output/'upstream'
    extracted.mkdir(exist_ok=True)
    with zipfile.ZipFile(archive) as source:
        for entry in source.infolist():
            target=(extracted/entry.filename).resolve()
            if not target.is_relative_to(extracted.resolve()):
                raise SystemExit('Unsafe source archive path')
        source.extractall(extracted)
    upstream=extracted/f'ps5-native-app-boilerplate-{COMMIT}'
native=upstream/'tooling/native'
host=output/'host';host.mkdir(exist_ok=True)
suffix='.exe' if os.name=='nt' else ''
tool=host/f'ps5-native-tool{suffix}'
runtime_builder=host/f'libc-builder{suffix}'
host_flags=['-std=c++20','-O2','-Wall','-Wextra']
if args.zlib_include:host_flags+=['-I',args.zlib_include.resolve()]
host_library=[args.zlib_library.resolve()] if args.zlib_library else ['-lz']
run([args.host_cxx,*host_flags,*[native/(n+'.cpp') for n in
     ['native_app_builder','self_container','elf_object','sce_module_writer']],*host_library,'-o',tool])
run([args.host_cxx,'-std=c++20','-O2','-Wall','-Wextra',native/'libc_builder.cpp','-o',runtime_builder])

obj=output/'obj';obj.mkdir(exist_ok=True)
target_flags=['-target','x86_64-sie-ps5','-std=c++20','-O2','-fvisibility-nodllstorageclass=default',
    '-nostdinc++','-isystem',sdk/'target/include/c++/v1','-isystem',sdk/'target/include',
    '-I',upstream/'src','-fno-stack-protector','-fno-plt','-femulated-tls',
    '-fno-exceptions','-fno-rtti','-ffunction-sections','-fdata-sections',
    '-resource-dir',llvm.parent/'lib/clang/18','-Wall','-Wextra']
objects=[]
for name,source in [('main',PROJECT/'native-launcher/main.cpp'),('demo_renderer',upstream/'src/demo_renderer.cpp'),
                    ('app_crt',native/'app_crt.cpp'),('app_cpp_runtime',native/'app_cpp_runtime.cpp')]:
    object_path=obj/(name+'.o')
    run([llvm/f'clang{suffix}',*target_flags,'-c',source,'-o',object_path])
    objects.append(object_path)
pie=output/'llvm-pie.elf'
run([llvm/f'ld.lld{suffix}','-m','elf_x86_64','-pie','-z','max-page-size=0x4000',
     '-T',native/'ps5-pie.ld','--eh-frame-hdr','--version-script',native/'app-symbols.map',
     '-e','_start','-o',pie,*objects,'--as-needed',*sorted((sdk/'target/lib').glob('*.so'))])
eboot=output/'eboot.elf'
run([tool,'link','--in',pie,'--out',eboot,'--stub-dir',sdk/'target/lib',
     '--module-sdk','0x02000009','--companion-sdk','0x08050001','--file-name','eboot.elf'])
app=output/'PPSA99741'
for path in ['sce_sys','sce_module','assets']:(app/path).mkdir(parents=True,exist_ok=True)
# A PS5 PKG uses the PS5 SELF magic. The upstream title-folder preset uses
# 0x1D3D154F for its mount workflow; that is not the container used here.
run([tool,'self','--sign','--in',eboot,'--out',app/'eboot.bin','--magic','0xEEF51454'])
raws=[]; signed=[]
for copy in ['a','b']:
    raw=output/f'libc-{copy}.raw.elf';fself=output/f'libc-{copy}.prx'
    run([runtime_builder,native/'runtime/api-surface.txt',native/'runtime/imports.txt',raw])
    run([tool,'self','--sign','--in',raw,'--out',fself])
    raws.append(raw.read_bytes());signed.append(fself.read_bytes())
if raws[0]!=raws[1] or signed[0]!=signed[1]:raise SystemExit('Runtime rebuilds differ')
if hashlib.sha256(raws[0]).hexdigest()!=RAW_RUNTIME_SHA256:raise SystemExit('Raw runtime checksum mismatch')
if hashlib.sha256(signed[0]).hexdigest()!=SIGNED_RUNTIME_SHA256:raise SystemExit('Signed runtime checksum mismatch')
(app/'sce_module/libc.prx').write_bytes(signed[0])
shutil.copyfile(PROJECT/'native-launcher/param.json',app/'sce_sys/param.json')
shutil.copyfile(PROJECT/'launcher/icon0.png',app/'sce_sys/icon0.png')
shutil.copyfile(payload,app/'assets/quentin-store.payload')
shutil.copyfile(upstream/'LICENSE',app/'assets/native-tooling-LICENSE.txt')
run([tool,'self','--inspect','--file',app/'eboot.bin'])
run([tool,'self','--inspect','--file',app/'sce_module/libc.prx'])
report={'upstream_commit':COMMIT,'upstream_archive_sha256':ARCHIVE_SHA256,
        'runtime_sha256':SIGNED_RUNTIME_SHA256,'payload_sha256':hashlib.sha256(payload.read_bytes()).hexdigest(),
        'eboot_sha256':hashlib.sha256((app/'eboot.bin').read_bytes()).hexdigest(),
        'hardware_validation':'Not tested on PS5 firmware 13.60'}
(output/'provenance.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Native application folder:',app)
