"""Build a PS4 FPKG launcher for etaHEN, with the PS5 ELF as a bundled asset."""
import argparse,pathlib,subprocess,shutil,os,xml.etree.ElementTree as ET
ROOT=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--toolchain',type=pathlib.Path,required=True);p.add_argument('--llvm',type=pathlib.Path,required=True);a=p.parse_args()
t=a.toolchain.resolve();llvm=a.llvm.resolve();build=ROOT/'build/pkg-launcher';build.mkdir(parents=True,exist_ok=True)
def run(args):subprocess.run([str(x) for x in args],cwd=build,check=True,env=dict(os.environ,DOTNET_ROLL_FORWARD='Major',OO_PS4_TOOLCHAIN=str(t)))
run([llvm/'clang.exe','--target=x86_64-pc-freebsd12-elf','-O2','-fPIC','-funwind-tables','-isystem',t/'include','-c',ROOT/'launcher/main.c','-o','launcher.o'])
run([llvm/'ld.lld.exe','-m','elf_x86_64','-pie','--script',t/'link.x','--eh-frame-hdr','-o','launcher.elf','-L',t/'lib',t/'lib/crt1.o','launcher.o','-lc','-lkernel','-lSceNet'])
bin=t/'bin/windows';run([bin/'create-fself.exe','-in','launcher.elf','--out','launcher.oelf','--eboot','eboot.bin','--paid','0x3800000000000011'])
for d in ['sce_sys/about','sce_module']: (build/d).mkdir(parents=True,exist_ok=True)
shutil.copy2(ROOT/'dist/quentin-store.elf',build/'quentin-store.elf')
shutil.copy2(t/'samples/hello_world/sce_module/libc.prx',build/'sce_module/libc.prx')
shutil.copy2(t/'samples/hello_world/sce_sys/about/right.sprx',build/'sce_sys/about/right.sprx')
shutil.copy2(ROOT/'launcher/icon0.png',build/'sce_sys/icon0.png')
shutil.copy2(ROOT/'LICENSE',build/'LICENSE.txt')
shutil.copy2(ROOT/'launcher/README.md',build/'README.txt')
pkg=bin/'PkgTool.Core.exe';run([pkg,'sfo_new','sce_sys/param.sfo'])
cid='IV0000-QSTR00001_00-QUENTINSTORE0000'
values={'APP_TYPE':('Integer',4,'1'),'APP_VER':('Utf8',8,'01.00'),'ATTRIBUTE':('Integer',4,'0'),'CATEGORY':('Utf8',4,'gd'),'CONTENT_ID':('Utf8',48,cid),'DOWNLOAD_DATA_SIZE':('Integer',4,'0'),'SYSTEM_VER':('Integer',4,'0'),'TITLE':('Utf8',128,'QUENTIN STORE Launcher'),'TITLE_ID':('Utf8',12,'QSTR00001'),'VERSION':('Utf8',8,'01.00')}
for name,(kind,size,value) in values.items():run([pkg,'sfo_setentry','sce_sys/param.sfo',name,'--type',kind,'--maxsize',size,'--value',value])
files=['eboot.bin','quentin-store.elf','sce_sys/param.sfo','sce_sys/icon0.png','sce_sys/about/right.sprx','sce_module/libc.prx','LICENSE.txt','README.txt']
(build/'third_party').mkdir(exist_ok=True)
for f in list((ROOT/'third_party').glob('*.txt'))+list((ROOT/'third_party').glob('*.LICENSE')):
    shutil.copy2(f,build/'third_party'/f.name);files.append('third_party/'+f.name)
shutil.copy2(t/'LICENSE',build/'third_party/OpenOrbis.LICENSE');files.append('third_party/OpenOrbis.LICENSE')
run([bin/'create-gp4.exe','-out','pkg.gp4','--content-id',cid,'--files',' '.join(files)])
project=ET.parse(build/'pkg.gp4');ET.SubElement(project.getroot().find('rootdir'),'dir',{'targ_name':'third_party'});project.write(build/'pkg.gp4',encoding='utf-8',xml_declaration=True)
run([pkg,'pkg_build','pkg.gp4','.'])
result=build/(cid+'.pkg');out=ROOT.parent/'QUENTIN-STORE-Launcher-1.00.pkg';shutil.copy2(result,out);run([pkg,'pkg_validate',out]);print(out)
