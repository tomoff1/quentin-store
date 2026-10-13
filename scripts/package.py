import argparse, hashlib, json, pathlib, shutil, zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
VERSION='0.1.2'
def main():
    p=argparse.ArgumentParser();p.add_argument('--repository',help='proprietaire/quentin-store');a=p.parse_args()
    build=ROOT/'build/ps5';out=ROOT/'dist';out.mkdir(exist_ok=True)
    for name in ['quentin-store','quentin-blocks']:
        f=build/(name+'.elf')
        if not f.exists() or f.read_bytes()[:4]!=b'\x7fELF':raise RuntimeError('ELF reel requis: '+str(f))
        shutil.copy2(f,out/f.name)
    items=[]
    if a.repository:
        parts=a.repository.split('/')
        if len(parts)!=2 or any(not s or any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._' for c in s) for s in parts): raise ValueError('Depot invalide')
        source='https://github.com/'+a.repository; base=source+'/releases/download/v'+VERSION+'/'
        for name,title,cat,desc in [('quentin-blocks','Quentin Blocks','Jeux','Jeu original de casse-briques. Stick gauche pour bouger, X pour jouer. Aucun jeu commercial ni ROM.'),('quentin-store','QUENTIN STORE','Applications','Catalogue de homebrews libres, telechargements HTTPS et SHA-256. Lancement manuel via ELF Loader.')]:
            f=out/(name+'.elf');items.append(dict(id=name,title=title,category=cat,description=desc,version=VERSION,url=base+f.name,sha256=hashlib.sha256(f.read_bytes()).hexdigest(),size=f.stat().st_size,license='GPL-3.0-or-later',source=source,redistribution=True,format='elf'))
    catalog={'schema':1,'items':items}
    (out/'catalog.json').write_text(json.dumps(catalog,indent=2,ensure_ascii=False),encoding='utf-8')
    (out/'config.json').write_text(json.dumps({'catalog_url':('https://raw.githubusercontent.com/'+a.repository+'/main/site/catalog.json') if a.repository else '', 'update_url':('https://raw.githubusercontent.com/'+a.repository+'/main/site/update.json') if a.repository else ''},indent=2))
    if items:
        (out/'update.json').write_text(json.dumps({'schema':1,'version':VERSION,'notes':'Diagnostics de demarrage, notifications et dossier de donnees PS5 fixe. Execution PS5 a confirmer. Installation manuelle par ELF Loader.','asset':items[1]},indent=2))
        for name in ['catalog.json','update.json']:shutil.copy2(out/name,ROOT/'site'/name)
        pkg=(out/'QUENTIN-STORE-PS5.pkg').exists()
        (ROOT/'site/release.json').write_text(json.dumps({'version':VERSION,'download':base+('QUENTIN-STORE-PS5.pkg' if pkg else 'quentin-store.elf'),'source':source,'format':'pkg-ps5' if pkg else 'elf'},indent=2)+'\n')
    else:
        print('Depot non configure: catalogue vide, aucune URL de telechargement inventee.')
    for f in ['ca-bundle.crt','font.ttf','LICENSE','README.md']:shutil.copy2(ROOT/f,out/f)
    target=out/('QUENTIN-STORE-'+VERSION+'.zip')
    relink=out/'relink';relink.mkdir(exist_ok=True)
    for f in build.glob('*.o'):shutil.copy2(f,relink/f.name)
    with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED) as z:
        for name in ['quentin-store.elf','quentin-blocks.elf','config.json','catalog.json','ca-bundle.crt','font.ttf','LICENSE','README.md']+(['update.json'] if items else []):z.write(out/name,name)
        for file in (ROOT/'third_party').glob('*.txt'):z.write(file,'third_party/'+file.name)
        for file in (ROOT/'third_party').glob('*.LICENSE'):z.write(file,'third_party/'+file.name)
        for file in (ROOT/'docs').glob('*.md'):z.write(file,'docs/'+file.name)
        z.write(ROOT/'scripts/send-elf.ps1','scripts/send-elf.ps1')
    source=out/('QUENTIN-STORE-'+VERSION+'-source.zip')
    with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as z:
        for name in ['src','native-launcher','native-pkg','tests','scripts','site','docs','third_party','.github']:
            for f in (ROOT/name).rglob('*'):
                if f.is_file() and not any(part in ('__pycache__','bin','obj') for part in f.parts):z.write(f,'QUENTIN-STORE/'+f.relative_to(ROOT).as_posix())
        for name in ['README.md','LICENSE','CMakeLists.txt','dependencies.lock.json','ca-bundle.crt','font.ttf','.gitignore']:z.write(ROOT/name,'QUENTIN-STORE/'+name)
        for f in relink.glob('*.o'):z.write(f,'QUENTIN-STORE/dist/relink/'+f.name)
        for f in out.glob('*.elf'):z.write(f,'QUENTIN-STORE/dist/'+f.name)
    checks={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in out.iterdir() if f.is_file() and f.suffix in ['.elf','.zip']}
    (out/'SHA256SUMS').write_text('\n'.join(h+'  '+n for n,h in checks.items())+'\n')
    print(target)
if __name__=='__main__':main()
