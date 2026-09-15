import {readFileSync, statSync, readdirSync} from 'node:fs';
import {resolve, dirname, relative} from 'node:path';
import {fileURLToPath} from 'node:url';
const root=resolve(dirname(fileURLToPath(import.meta.url)), '../apps/website');
const catalog=JSON.parse(readFileSync(resolve(root,'assets/data/beat_catalog.json'),'utf8'));
const references=new Set();
function add(value) {
  if (!value || /^(?:https?:|data:|#|mailto:)/.test(value)) return;
  const path=decodeURIComponent(value.split(/[?#]/)[0]).replace(/^\/?api\/v1\/assets\//,'assets/').replace(/^\//,'');
  if(path.startsWith('assets/'))references.add(path);
}
for(const track of catalog) {
  for(const field of ['public_url','stream_url','full_song_url','artwork_url']) add(track[field]);
  if(track.asset_file_name)add('assets/audio/catalog/'+track.asset_file_name);
}
for(const file of ['index.html','styles.css','app.js','config.js','ops.html','ops.css','ops.js']) {
 const content=readFileSync(resolve(root,file),'utf8');
 for(const m of content.matchAll(/(?:["'(])((?:\/?assets\/)[^"'<>\s)]+)/g))if(!m[1].includes('${'))add(m[1]);
}
for(const path of references) {
 const absolute=resolve(root,path);
 if(!absolute.startsWith(root+'/')&&!absolute.startsWith(root+'\\'))throw new Error('Asset escapes website root');
 if(!statSync(absolute).isFile() || statSync(absolute).size===0)throw new Error('Missing or empty asset: '+path);
 let current=root;
 for(const part of path.split('/')) {
   if(!readdirSync(current).includes(part))throw new Error('Case mismatch: '+path);
   current=resolve(current,part);
 }
 if(statSync(absolute).size>25*1024*1024)throw new Error('Asset exceeds Pages size limit: '+relative(root,absolute));
}
console.log(`Website asset graph verified: ${catalog.length} real tracks, ${references.size} referenced files.`);
