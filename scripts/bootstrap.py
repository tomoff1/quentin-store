"""Dependances publiques epinglees, sans installation systeme. Python >= 3.10."""
import hashlib, json, os, pathlib, shutil, sys, tarfile, urllib.request, zipfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
DEPS = ROOT / '.deps'
LOCK = json.loads((ROOT / 'dependencies.lock.json').read_text())
URLS = {
    'sdk': 'https://github.com/ps5-payload-dev/sdk/releases/download/v0.44/ps5-payload-sdk.zip',
    'ports': 'https://github.com/ps5-payload-dev/pacbrew-repo/releases/download/v0.41.1/ps5-payload-dev.tar.gz',
    'llvm': 'https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/clang%2Bllvm-18.1.8-x86_64-pc-windows-msvc.tar.xz',
}
def fetch(name):
    target = DEPS / (name + ('.zip' if name == 'sdk' else '.tar.gz' if name == 'ports' else '.tar.xz'))
    if not target.exists():
        print('Telechargement:', name, flush=True)
        urllib.request.urlretrieve(URLS[name], target)
    with target.open('rb') as f: digest = hashlib.file_digest(f, 'sha256').hexdigest() if hasattr(hashlib,'file_digest') else hashlib.sha256(f.read()).hexdigest()
    if digest != LOCK[name]: raise RuntimeError('SHA256 dependance incorrect: ' + name)
    return target
def extract_tar(path, dest, choose):
    dest = dest.resolve()
    with tarfile.open(path) as archive:
        for member in archive:
            if not choose(member.name) or not member.isfile(): continue
            target = (dest / member.name).resolve()
            if dest not in target.parents: raise RuntimeError('Chemin archive dangereux')
            target.parent.mkdir(parents=True, exist_ok=True)
            with archive.extractfile(member) as src, target.open('wb') as out: shutil.copyfileobj(src,out)
def main():
    DEPS.mkdir(exist_ok=True)
    with zipfile.ZipFile(fetch('sdk')) as z:
        for m in z.infolist():
            if m.is_dir(): continue
            target=(DEPS/m.filename).resolve()
            if DEPS.resolve() not in target.parents: raise RuntimeError('Chemin ZIP dangereux')
            target.parent.mkdir(parents=True,exist_ok=True)
            target.write_bytes(z.read(m))
    extract_tar(fetch('ports'), DEPS/'ports', lambda n: n.startswith('opt/ps5-payload-sdk/target/user/homebrew/'))
    if os.name == 'nt':
        extract_tar(fetch('llvm'), DEPS/'llvm', lambda n: n.endswith(('/bin/clang.exe','/bin/ld.lld.exe','/bin/llvm-readelf.exe')) or ('/lib/clang/' in n and '/include/' in n))
    print('SDK et ports installes dans',DEPS)
if __name__ == '__main__': main()
