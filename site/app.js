// Active le bouton uniquement si la release existe publiquement.
(async()=>{
  try {
    const meta=await fetch('release.json'); if(!meta.ok)return;
    const release=await meta.json();
    const response=await fetch('https://api.github.com/repos/tomoff1/quentin-store/releases/tags/v'+release.version);
    if(!response.ok)return;
    const remote=await response.json();
    const asset=remote.assets.find(a=>a.browser_download_url===release.download);
    if(!asset)return;
    const button=document.querySelector('#download');button.href=asset.browser_download_url;
    button.textContent='Télécharger v'+release.version;button.classList.remove('disabled');button.removeAttribute('aria-disabled');
  }catch{ /* La page reste lisible hors ligne; aucun faux lien actif. */ }
})();
