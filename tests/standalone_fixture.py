"""Run the real desktop client in a fresh directory, with no sidecar assets."""
import argparse,json,os,pathlib,subprocess,tempfile
p=argparse.ArgumentParser();p.add_argument('--binary',required=True);p.add_argument('--work');a=p.parse_args()
binary=pathlib.Path(a.binary).resolve();root=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='qs-standalone-',dir=a.work) as tmp:
    data=pathlib.Path(tmp)/'data';capture=pathlib.Path(tmp)/'frame.bmp'
    env=dict(os.environ,SDL_VIDEODRIVER='dummy',QS_CAPTURE=str(capture))
    subprocess.run([str(binary),str(data)],env=env,check=True,timeout=45)
    assert capture.stat().st_size>1280*720*3
    assert (data/'ca-bundle.crt').read_bytes()==(root/'ca-bundle.crt').read_bytes()
    config=json.loads((data/'config.json').read_text());assert config['catalog_url'].startswith('https://')
    assert json.loads((data/'catalog.json').read_text())['items']
    assert not (data/'font.ttf').exists(), 'Font must load directly from embedded memory'
    custom={'catalog_url':'','update_url':''};(data/'config.json').write_text(json.dumps(custom))
    history='[]';(data/'downloads/history.json').write_text(history)
    subprocess.run([str(binary),str(data)],env=env,check=True,timeout=45)
    assert json.loads((data/'config.json').read_text())==custom
    assert (data/'downloads/history.json').read_text()==history
print('Standalone startup, embedded CA/catalog/font and existing settings verified.')
