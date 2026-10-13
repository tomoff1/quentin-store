"""Authentification Git en memoire ; aucune URL contenant un token ni modification globale."""
import base64,os,subprocess
token=subprocess.run(['gh','auth','token','--hostname','github.com'],check=True,capture_output=True,text=True).stdout.strip()
if not token:raise RuntimeError('Connexion GitHub requise')
env=os.environ.copy();env['GIT_CONFIG_COUNT']='1';env['GIT_CONFIG_KEY_0']='http.https://github.com/.extraheader'
env['GIT_CONFIG_VALUE_0']='AUTHORIZATION: basic '+base64.b64encode(('x-access-token:'+token).encode()).decode()
subprocess.run(['git','-c','credential.helper=','push','-u','origin','main'],env=env,check=True)
