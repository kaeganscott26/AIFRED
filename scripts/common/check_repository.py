"""Read-only construction checks; no imports from product runtime or network access."""
from pathlib import Path
import json,re,subprocess,sys
from urllib.parse import unquote
ROOT=Path(__file__).resolve().parents[2]
def names():
 return sorted(set(subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=ROOT).decode().split('\0'))-{''})
def check():
 failures=[]
 tracked=names()
 for name in tracked:
  p=ROOT/name
  if not p.is_file():continue
  if name.startswith('out/'):failures.append('Generated output is tracked: '+name)
  if p.suffix=='.md' and not name.startswith('.obsidian/plugins/'):
   text=re.sub(r'```.*?```','',p.read_text(encoding='utf-8',errors='replace'),flags=re.S)
   for link in re.findall(r'\[[^\]]*\]\(([^\n)]+)\)',text):
    target=unquote(link.split('#')[0].strip('<>'))
    if not target or re.match(r'\w+:|//',target):continue
    if not (p.parent/target).exists():failures.append(f'Broken link: {name} -> {target}')
 presets=json.loads((ROOT/'CMakePresets.json').read_text())
 expected={'windows-release':'windows-x64','macos-release':'macos-arm64','linux-release':'linux-x64'}
 for preset in presets['configurePresets']:
  if preset.get('hidden'):continue
  key=expected.pop(preset['name'],None)
  if key is None or preset['binaryDir']!='${sourceDir}/out/'+key+'/build':failures.append('Noncanonical CMake preset: '+preset['name'])
 if expected:failures.append('Missing platform presets: '+str(expected))
 for name in ('README.md','docs/ARCHITECTURE.md','docs/BUILD.md','docs/TESTING.md','docs/DEVELOPMENT.md','docs/INSTALLATION.md','docs/DISTRIBUTION.md','docs/COEXISTENCE.md','shared-dsp/README.md'):
  if not (ROOT/name).is_file():failures.append('Missing authority: '+name)
 # Local source dependencies must never reach a sibling checkout.
 for name in tracked:
  p=ROOT/name
  if p.is_file() and (p.name=='CMakeLists.txt' or p.suffix in ('.cmake','.csproj','.props')):
   content=p.read_text(encoding='utf-8',errors='replace')
   if re.search(r'[A-Za-z]:[\\/]+Users[\\/]|/home/[^$\s]+|Documents[\\/]+Projects',content):failures.append('Machine source/build path: '+name)
   if re.search(r'(add_subdirectory|ProjectReference)[^\n]*(AIFRED_Official-|BETA[/\\]AIFRED)',content):failures.append('Sibling source dependency: '+name)
 for failure in failures:print(failure,file=sys.stderr)
 if not failures:print(f'Construction checks passed ({len(tracked)} source/config paths inspected).')
 return int(bool(failures))
if __name__=='__main__':sys.exit(check())
