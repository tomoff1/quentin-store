"""Compilation croisee C++ puis ELF PIE avec le script du SDK public."""
import argparse, os, pathlib, shutil, subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
def run(args):
    print(' '.join(str(x) for x in args),flush=True)
    subprocess.run([str(x) for x in args],check=True)
def main():
    p=argparse.ArgumentParser();p.add_argument('--sdk',type=pathlib.Path,default=ROOT/'.deps/ps5-payload-sdk');p.add_argument('--ports',type=pathlib.Path,default=ROOT/'.deps/ports/opt/ps5-payload-sdk/target/user/homebrew');p.add_argument('--llvm',type=pathlib.Path);a=p.parse_args()
    sdk=a.sdk.resolve();hb=a.ports.resolve()
    llvm=a.llvm
    if llvm is None and os.name=='nt': llvm=next((ROOT/'.deps/llvm').glob('*/bin'))
    if llvm: llvm=llvm.resolve()
    clang=llvm/('clang.exe' if os.name=='nt' else 'clang') if llvm else shutil.which('clang-18') or shutil.which('clang')
    linker=llvm/('ld.lld.exe' if os.name=='nt' else 'ld.lld') if llvm else shutil.which('ld.lld-18') or shutil.which('ld.lld')
    if not clang or not linker: raise RuntimeError('Installer clang-18 et lld-18, ou lancer bootstrap.py sur Windows')
    build=ROOT/'build/ps5';build.mkdir(parents=True,exist_ok=True)
    flags=['-target','x86_64-sie-ps5','-std=c++17','-O2','-g','-D__PROSPERO__','-fno-stack-protector','-fno-plt','-femulated-tls','-fvisibility-nodllstorageclass=default','-nostdinc++','-isystem',sdk/'target/include/c++/v1','-isystem',sdk/'target/include','-isystem',hb/'include','-isystem',hb/'include/SDL2','-I',ROOT/'third_party','-I',ROOT/'src']
    if llvm and os.name=='nt': flags += ['-resource-dir',llvm.parent/'lib/clang/18']
    flags += ['-fexceptions','-frtti']
    common=['SDL2_gfx','SDL2_ttf','freetype','png16','bz2','SDL2','iconv','samplerate','SceSystemService','SceUserService','ScePad','SceAudioOut','SceVideoOut','SceKeyboard','SceImeDialog']
    for name,sources,extra in [('quentin-store',['main','network'],['curl','psl','ssl','crypto','zstd','z']),('quentin-blocks',['blocks'],[])]:
        objs=[]
        for src in sources:
            obj=build/(src+'.o');run([clang,*flags,'-c',ROOT/'src'/(src+'.cpp'),'-o',obj]);objs.append(obj)
        run([linker,'-m','elf_x86_64','-pie','--eh-frame-hdr','-z','max-page-size=0x4000','-T',sdk/'ldscripts/elf_x86_64.x','-o',build/(name+'.elf'),sdk/'target/lib/crt1.o',*objs,'-L',sdk/'target/lib','-L',hb/'lib','--start-group',*['-l'+v for v in common+extra+['c++','c++abi','unwind','c','kernel_web','SceLibcInternal','SceNet','SceSysmodule','SceNetCtl']], '--end-group'])
    print('Compilation terminee. Aucune execution PS5 effectuee.')
if __name__=='__main__':main()
