"""Relier les objets fournis avec des bibliotheques modifiees (dont libiconv LGPL)."""
import argparse, os, pathlib, subprocess, shutil
ROOT=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sdk',type=pathlib.Path,default=ROOT/'.deps/ps5-payload-sdk');p.add_argument('--ports',type=pathlib.Path,default=ROOT/'.deps/ports/opt/ps5-payload-sdk/target/user/homebrew');p.add_argument('--linker');a=p.parse_args()
linker=a.linker or (str(next((ROOT/'.deps/llvm').glob('*/bin/ld.lld.exe'))) if os.name=='nt' else shutil.which('ld.lld-18') or shutil.which('ld.lld'))
sdk=a.sdk.resolve();hb=a.ports.resolve();objects=ROOT/'dist/relink';out=ROOT/'build/ps5';out.mkdir(parents=True,exist_ok=True)
common=['SDL2_gfx','SDL2_ttf','freetype','png16','bz2','SDL2','iconv','samplerate','SceSystemService','SceUserService','ScePad','SceAudioOut','SceVideoOut','SceKeyboard','SceImeDialog']
for name,objs,extra in [('quentin-store',['main.o','network.o'],['curl','psl','ssl','crypto','zstd','z']),('quentin-blocks',['blocks.o'],[])]:
    args=[linker,'-m','elf_x86_64','-pie','--eh-frame-hdr','-z','max-page-size=0x4000','-T',sdk/'ldscripts/elf_x86_64.x','-o',out/(name+'.elf'),sdk/'target/lib/crt1.o',*[objects/o for o in objs],'-L',sdk/'target/lib','-L',hb/'lib','--start-group',*['-l'+v for v in common+extra+['c++','c++abi','unwind','c','kernel_web','SceLibcInternal','SceNet','SceSysmodule','SceNetCtl']],'--end-group']
    subprocess.run([str(x) for x in args],check=True)
print('ELF relie avec les bibliotheques selectionnees.')
