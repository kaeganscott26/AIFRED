"""Developer release assembly. No audio analysis, provider calls or installed-user-data access."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[2]

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def checked_path(path, parent):
    path, parent = Path(path).absolute(), Path(parent).absolute()
    if path == parent or not path.is_relative_to(parent):
        raise ValueError(f'Path must be a child of {parent}: {path}')
    for part in (parent, *path.relative_to(parent).parents):
        candidate = part if part.is_absolute() else parent / part
        if candidate.is_symlink() or (hasattr(candidate, 'is_junction') and candidate.is_junction()):
            raise ValueError(f'Reparse point is not permitted: {candidate}')
    if path.exists():
        for candidate in [path, *path.rglob('*')]:
            if candidate.is_symlink() or (hasattr(candidate, 'is_junction') and candidate.is_junction()):
                raise ValueError(f'Reparse point is not permitted: {candidate}')
    if not path.resolve().is_relative_to(parent.resolve()):
        raise ValueError('Resolved path escaped its owner')
    return path

def recycle(path, parent):
    path = checked_path(path, parent)
    if not path.exists():
        return
    if sys.platform == 'win32':
        env = dict(os.environ, AIFRED_RECYCLE_TARGET=str(path))
        command = "Add-Type -AssemblyName Microsoft.VisualBasic; [Microsoft.VisualBasic.FileIO.FileSystem]::DeleteDirectory($env:AIFRED_RECYCLE_TARGET, [Microsoft.VisualBasic.FileIO.UIOption]::OnlyErrorDialogs, [Microsoft.VisualBasic.FileIO.RecycleOption]::SendToRecycleBin, [Microsoft.VisualBasic.FileIO.UICancelOption]::ThrowException)"
        subprocess.run(['powershell.exe', '-NoProfile', '-NonInteractive', '-Command', command], env=env, check=True)
    elif sys.platform == 'darwin':
        script = 'on run argv\ntell application "Finder" to delete POSIX file (item 1 of argv)\nend run'
        subprocess.run(['osascript', '-e', script, str(path)], check=True)
    elif shutil.which('gio'):
        subprocess.run(['gio','trash',str(path)],check=True)
    else:
        # A recoverable external quarantine is the fallback; never unlink recursively.
        quarantine=Path(tempfile.gettempdir())/'aifred-generated-quarantine'
        quarantine.mkdir(exist_ok=True)
        target=quarantine/(path.name+'-'+uuid.uuid4().hex)
        shutil.move(str(path), str(target))
        (quarantine/(target.name+'.json')).write_text(json.dumps({'original':str(path),'quarantine':str(target)}))
        print(f'Trash unavailable; retained generated output at {target}')
    if path.exists():
        raise RuntimeError(f'Recycling did not remove the original path: {path}')

def layout():
    return json.loads((ROOT/'scripts/release-layout.json').read_text())

def owned(folder):
    marker = folder/'.aifred-stage.json'
    info = json.loads(marker.read_text())
    expected=layout()
    if info != {'product':expected['product'],'channel':expected['channel']}:
        raise ValueError('Artifact belongs to a different product/channel')

def prepare(key):
    base=ROOT/'out'/key
    stage=checked_path(base/'stage',ROOT/'out')
    if stage.exists():
        owned(stage)
        recycle(stage,ROOT/'out')
    stage.mkdir(parents=True)
    info=layout()
    (stage/'.aifred-stage.json').write_text(json.dumps({'product':info['product'],'channel':info['channel']}))

def git(*args):
    return subprocess.check_output(['git',*args],cwd=ROOT,text=True).strip()

def source_tree_hash():
    names=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=ROOT).decode().split('\0')
    hashes={name:digest(ROOT/name) for name in sorted(set(names)) if name and (ROOT/name).is_file()}
    return hashlib.sha256(json.dumps(hashes,sort_keys=True).encode()).hexdigest()

def prepare_scratch(key):
    folder=checked_path(ROOT/'out'/key/'build'/'packaging',ROOT/'out')
    if folder.exists():
        owned(folder)
        recycle(folder,ROOT/'out')
    folder.mkdir(parents=True)
    info=layout()
    (folder/'.aifred-stage.json').write_text(json.dumps({'product':info['product'],'channel':info['channel']}))

def manifest(key):
    if key!='windows-x64': raise ValueError('Full release manifest/promotion is Windows-only; other platforms are SCAFFOLDED / NOT VALIDATED')
    stage=checked_path(ROOT/'out'/key/'stage',ROOT/'out')
    owned(stage)
    info=layout()
    cmake=(ROOT/'CMakeLists.txt').read_text()
    match=re.search(r'set\(AIFRED_VERSION_STRING\s+"([^"]+)"',cmake) or re.search(r'project\(AIFRED VERSION ([\d.]+)',cmake)
    if not match: raise ValueError('Version missing from CMake authority')
    files={p.relative_to(stage).as_posix():digest(p) for p in stage.rglob('*') if p.is_file() and p.name!='manifest.json'}
    is_official=info['product']=='AIFRED 4'
    plugin='Aifred.vst3' if is_official else 'AIFRED-VST3-windows/Aifred.vst3'
    engine='AifredIntelligenceHost' if is_official else 'AIFRED-VST3-windows/AifredIntelligenceHost'
    result={'schema':'aifred.release.v2','product':info['product'],'channel':info['channel'],'version':match.group(1),'gitSha':git('rev-parse','HEAD'),'workingTreeDirty':bool(git('status','--porcelain')),'sourceTreeSha256':source_tree_hash(),'platform':key,'architecture':info['platforms'][key]['architecture'],'toolchain':{'host':platform.platform(),'cmake':subprocess.check_output(['cmake','--version'],text=True).splitlines()[0],'dotnet':subprocess.check_output(['dotnet','--version'],text=True).strip()},'dspProfileSchemaVersion':info['dspProfileSchemaVersion'],'sharedCoreVersion':info['sharedCoreVersion'],'profiles':info['profiles'],'contextSchema':info['contextSchema'],'runtimeChannel':info['runtimeChannel'],'hostPort':info['hostPort'],'plugin':plugin,'engine':engine,'installer':None if is_official else 'installer/AIFRED-VST3-Setup.exe','hashes':files,'validation':'build and repository tests; DAW/signing not certified'}
    (stage/'manifest.json').write_text(json.dumps(result,indent=2)+'\n')

def verify(key,location='current'):
    if key!='windows-x64': raise ValueError('Complete release validation is implemented for Windows only')
    folder=checked_path(ROOT/'out'/key/location,ROOT/'out')
    owned(folder)
    data=json.loads((folder/'manifest.json').read_text())
    info=layout()
    if (data['product'],data['channel'],data['platform']) != (info['product'],info['channel'],key):
        raise ValueError('Manifest identity mismatch')
    actual={p.relative_to(folder).as_posix():digest(p) for p in folder.rglob('*') if p.is_file() and p.name!='manifest.json'}
    if actual!=data['hashes']: raise ValueError('Artifact inventory/hash mismatch')
    if key=='windows-x64':
        checked_path(folder/data['plugin'],folder)
        checked_path(folder/data['engine'],folder)
        required=[data['plugin']+'/Contents/x86_64-win/Aifred.vst3',data['plugin']+'/Contents/Resources/moduleinfo.json']
        if data['schema']=='aifred.release.v2':
            required += [data['engine']+'/AifredIntelligenceHost'+suffix for suffix in ('.exe','.dll','.runtimeconfig.json')]
            required += [data['engine']+'/channel.json']
            if json.loads((folder/data['engine']/'channel.json').read_text(encoding='utf-8-sig')) != {'channel':info['runtimeChannel']}: raise ValueError('Host channel ownership mismatch')
        elif data['schema']!='aifred.release.v1': raise ValueError('Unknown artifact manifest schema')
        if info['product']!='AIFRED 4': required += ['AIFRED-VST3-windows.zip','installer/AIFRED-VST3-Setup.exe','uninstaller/AIFRED-Uninstall.exe']
        for name in required:
            if name not in actual: raise ValueError(f'Required component missing: {name}')
        source=ROOT/'out'/key/'build'/info['platforms'][key]['plugin']/'Contents/x86_64-win/Aifred.vst3'
        if location=='stage' and digest(source)!=actual[data['plugin']+'/Contents/x86_64-win/Aifred.vst3']:
            raise ValueError('Staged plugin differs from the exact build target')
    print(f'Verified {folder}: {len(actual)} hashed files')

def promote(key):
    verify(key,'stage')
    base=ROOT/'out'/key
    current,stage,previous=(checked_path(base/n,ROOT/'out') for n in ('current','stage','previous'))
    if previous.exists():
        raise ValueError('Retained previous release requires explicit recovery/recycling before another promotion')
    if current.exists():
        verify(key,'current')
        current.rename(previous)
    try:
        stage.rename(current)
        verify(key,'current')
    except BaseException:
        if current.exists(): current.rename(stage)
        if previous.exists(): previous.rename(current)
        raise
    if previous.exists():
        try: recycle(previous,ROOT/'out')
        except Exception as error: print(f'New current verified; previous retained for recovery: {error}',file=sys.stderr)

def rollback(key):
    base=ROOT/'out'/key
    verify(key,'previous')
    if (base/'current').exists():
        raise ValueError('Rollback requires current to be absent; preserve and inspect it before recovery')
    checked_path(base/'previous',ROOT/'out').rename(base/'current')
    verify(key)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=['prepare','prepare_scratch','manifest','verify','promote','rollback'])
    parser.add_argument('--platform',choices=['windows-x64','macos-arm64','linux-x64'],required=True)
    parser.add_argument('--location',choices=['stage','current','previous'],default='current')
    args=parser.parse_args()
    if args.action=='verify': verify(args.platform,args.location)
    else: globals()[args.action](args.platform)
