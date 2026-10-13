"""Generate deterministic embedded resources; no network needed."""
import pathlib,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
def generate(target):
    target=pathlib.Path(target);target.parent.mkdir(parents=True,exist_ok=True)
    chunks=['#pragma once\nnamespace qs { namespace embedded {\n']
    for name,file in [('font','font.ttf'),('ca','ca-bundle.crt'),('catalog','src/catalog.seed.json')]:
        data=(ROOT/file).read_bytes()
        chunks.append('inline constexpr unsigned char '+name+'[] = {\n')
        for offset in range(0,len(data),24):chunks.append(','.join(str(b) for b in data[offset:offset+24])+',\n')
        chunks.append('};\n')
    chunks.append('}}\n');target.write_text(''.join(chunks),encoding='ascii')
if __name__=='__main__':generate(sys.argv[1])
