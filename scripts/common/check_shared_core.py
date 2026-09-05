"""Verify the pinned shared source inventory; optional peer comparison is never a build dependency."""
import argparse, hashlib, json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
def inventory(root):
    result = {}
    for directory in ('shared-dsp', 'tools/AifredIntelligenceHost', 'tools/AifredIntelligenceHost.Tests'):
        for path in (root / directory).rglob('*'):
            if path.is_file():
                result[path.relative_to(root).as_posix()] = hashlib.sha256(path.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    return result
if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--peer',type=Path);args=parser.parse_args()
    lock=json.loads((ROOT/'shared-core.lock.json').read_text())
    actual=inventory(ROOT)
    if actual != lock['files']: raise SystemExit('Shared source differs from shared-core.lock.json; review and version both channels together.')
    if args.peer and actual != inventory(args.peer.resolve()): raise SystemExit('Shared source parity failed.')
    print(f"Shared core {lock['version']}: verified {len(actual)} files" + (' and peer parity' if args.peer else ''))
