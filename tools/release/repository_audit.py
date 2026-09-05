"""Current read-only developer inventories; generated reports belong under out/."""
from pathlib import Path
import argparse,collections,json,platform,re,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
REQUIRED={
 'website':['apps/website/'+p for p in ('_worker.js','functions/api/v1/[[path]].js','functions/api/[[path]].js','functions/ws/chat.js','index.html','app.js','config.js','styles.css','assets/data/beat_catalog.json','wrangler.toml')],
 'admin':['apps/admin-android/'+p for p in ('settings.gradle.kts','build.gradle.kts','app/build.gradle.kts','gradlew','app/src/main/AndroidManifest.xml')],
}
def main(kind):
 parser=argparse.ArgumentParser(description=__doc__);group=parser.add_mutually_exclusive_group();group.add_argument('--check',action='store_true');group.add_argument('--stdout',action='store_true');args=parser.parse_args()
 paths=[p for p in subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode().split('\0') if p and (ROOT/p).exists()]
 errors=[];details={}
 if kind in REQUIRED:
  for path in REQUIRED[kind]:
   if not (ROOT/path).is_file():errors.append('Missing current source: '+path)
  details['requiredFiles']=REQUIRED[kind]
 elif kind=='workflow':
  forbidden=['wrangler deploy','pages deploy','CLOUDFLARE_API_TOKEN','CF_API_TOKEN','gh release create','gh release upload','git push','upload-artifact','download-artifact','secrets.']
  for name in ['aifred-monorepo-validate.yml','aifred-website-preview-dryrun.yml']:
   p=ROOT/'.github/workflows'/name;text=p.read_text();details[name]={'manual':bool(re.search(r'\n  workflow_dispatch:',text)),'readOnlyPermissions':'contents: read' in text}
   if not all(details[name].values()):errors.append('Read-only workflow contract changed: '+name)
   for token in forbidden:
    if token in text:errors.append(f'Forbidden side effect/secret reference in {name}: {token}')
  text=(ROOT/'.github/workflows/build.yml').read_text()
  for token in ("github.event_name == 'workflow_dispatch' && github.ref == 'refs/heads/main'",'CLOUDFLARE_CREDENTIALS_PRESENT',"startsWith(github.ref, 'refs/tags/v')",'./scripts/windows/build.ps1 -Action release','out/windows-x64/current/'):
   if token not in text:errors.append('Missing release/deployment gate or canonical path: '+token)
  details['buildWorkflow']='Windows validated-current pipeline; macOS candidate CI; tag releases and manual credential-gated website deployment'
 else:
  details['trackedFiles']=paths;details['areas']=dict(collections.Counter(p.split('/')[0] for p in paths))
  for p in paths:
   if p.startswith(('out/','build/','dist/')):errors.append('Tracked generated product output: '+p)
 report={'kind':kind,'gitSha':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'status':'FAIL' if errors else 'PASS','errors':errors,'details':details}
 text=json.dumps(report,indent=2)+'\n'
 if args.check:print(kind+': '+report['status']);[print(e,file=sys.stderr) for e in errors]
 elif args.stdout:print(text,end='')
 else:
  key={'Windows':'windows-x64','Darwin':'macos-arm64'}.get(platform.system(),'linux-x64');dest=ROOT/'out'/key/'build/reports'/('repository-'+kind+'.json');dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(text);print(dest)
 return int(bool(errors))
