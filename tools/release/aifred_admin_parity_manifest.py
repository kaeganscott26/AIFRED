#!/usr/bin/env python3
"""Generate a local AIFRED Phase 8 Android admin parity manifest."""

from __future__ import annotations

import argparse
import datetime as _dt
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REPORT_PATH = ROOT / "docs" / "operations" / "PHASE8_ADMIN_PARITY_MANIFEST.md"

OLD_ROOT = ROOT / "android_admin"
NEW_ROOT = ROOT / "apps" / "admin-android"

CRITICAL_ITEMS = [
    "settings.gradle.kts",
    "build.gradle.kts",
    "gradlew",
    "app/build.gradle.kts",
    "app/src/main/AndroidManifest.xml",
    "app/src/main/java or app/src/main/kotlin",
]

REFERENCES = [
    "north3rnlight3r.com",
    "/api/v1",
    "/api/v1/admin",
    "login",
    "Bearer",
    "versionName",
    "versionCode",
]

EXCLUDED_DIRS = {".git", "node_modules", ".wrangler", ".gradle", "build", "dist", "cache"}
MAX_TEXT_SCAN_BYTES = 5 * 1024 * 1024


def git_output(*args: str) -> str:
    try:
        return subprocess.check_output(
            ["git", *args],
            cwd=ROOT,
            text=True,
            stderr=subprocess.DEVNULL,
        ).strip()
    except Exception:
        return "unknown"


def generated_at() -> str:
    return _dt.datetime.now().astimezone().isoformat(timespec="seconds")


def format_bytes(size: int) -> str:
    value = float(size)
    for unit in ("B", "KB", "MB", "GB"):
        if value < 1024.0 or unit == "GB":
            if unit == "B":
                return f"{int(value)} {unit}"
            return f"{value:.1f} {unit}"
        value /= 1024.0
    return f"{size} B"


def rel_to_repo(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def rel_to_root(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def iter_files(root: Path) -> list[Path]:
    if not root.is_dir():
        return []
    files: list[Path] = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [name for name in dirnames if name not in EXCLUDED_DIRS]
        base = Path(dirpath)
        for filename in filenames:
            files.append(base / filename)
    return sorted(files, key=lambda item: rel_to_root(item, root))


def file_map(root: Path) -> dict[str, Path]:
    return {rel_to_root(path, root): path for path in iter_files(root)}


def is_text_scan_candidate(path: Path) -> bool:
    try:
        if path.stat().st_size > MAX_TEXT_SCAN_BYTES:
            return False
        data = path.read_bytes()[:4096]
    except OSError:
        return False
    return b"\0" not in data


def reference_counts(root: Path) -> dict[str, int]:
    counts = {ref: 0 for ref in REFERENCES}
    for path in iter_files(root):
        if not is_text_scan_candidate(path):
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        lower = text.lower()
        for ref in REFERENCES:
            if ref in {"Bearer", "versionName", "versionCode"}:
                counts[ref] += text.count(ref)
            else:
                counts[ref] += lower.count(ref.lower())
    return counts


def item_exists(root: Path, item: str) -> bool:
    if item == "app/src/main/java or app/src/main/kotlin":
        main = root / "app" / "src" / "main"
        return (main / "java").is_dir() or (main / "kotlin").is_dir()
    return (root / item).exists()


def item_size(root: Path, item: str) -> str:
    if item == "app/src/main/java or app/src/main/kotlin":
        return "directory" if item_exists(root, item) else "n/a"
    path = root / item
    if not path.is_file():
        return "n/a"
    return format_bytes(path.stat().st_size)


def append_path_list(lines: list[str], paths: list[str]) -> None:
    if not paths:
        lines.append("- None.")
        return
    for path in paths:
        lines.append(f"- `{path}`")


def build_report() -> str:
    old_files = file_map(OLD_ROOT)
    new_files = file_map(NEW_ROOT)
    old_paths = set(old_files)
    new_paths = set(new_files)
    shared_paths = sorted(old_paths & new_paths)
    only_old = sorted(old_paths - new_paths)
    only_new = sorted(new_paths - old_paths)
    old_refs = reference_counts(OLD_ROOT)
    new_refs = reference_counts(NEW_ROOT)
    new_shape_ready = NEW_ROOT.is_dir() and all(item_exists(NEW_ROOT, item) for item in CRITICAL_ITEMS)

    lines: list[str] = []
    lines.append("# Phase 8 Android Admin Parity Manifest")
    lines.append("")
    lines.append(f"Timestamp: {generated_at()}")
    lines.append("")
    lines.append(f"Git branch: `{git_output('branch', '--show-current')}`")
    lines.append(f"Git commit: `{git_output('rev-parse', '--short', 'HEAD')}`")
    lines.append("")
    lines.append("## Roots")
    lines.append("")
    lines.append(f"- Old root: `{rel_to_repo(OLD_ROOT)}`")
    lines.append(f"- New root: `{rel_to_repo(NEW_ROOT)}`")
    lines.append("")
    lines.append("## File Counts")
    lines.append("")
    lines.append(f"- Total file count in `android_admin/`: {len(old_files)}")
    lines.append(f"- Total file count in `apps/admin-android/`: {len(new_files)}")
    lines.append(f"- Shared relative paths count: {len(shared_paths)}")
    lines.append("")
    lines.append("## Files Only In `android_admin/`")
    lines.append("")
    append_path_list(lines, only_old)
    lines.append("")
    lines.append("## Files Only In `apps/admin-android/`")
    lines.append("")
    append_path_list(lines, only_new)
    lines.append("")
    lines.append("## Critical Gradle And Source Shape Comparison")
    lines.append("")
    lines.append("| Relative path | `android_admin/` present | `apps/admin-android/` present | Old size | New size |")
    lines.append("| --- | --- | --- | ---: | ---: |")
    for item in CRITICAL_ITEMS:
        lines.append(
            f"| `{item}` | {'yes' if item_exists(OLD_ROOT, item) else 'no'} | {'yes' if item_exists(NEW_ROOT, item) else 'no'} | {item_size(OLD_ROOT, item)} | {item_size(NEW_ROOT, item)} |"
        )
    lines.append("")
    lines.append("## Backend And Admin Reference Counts")
    lines.append("")
    lines.append("| Reference | `android_admin/` | `apps/admin-android/` |")
    lines.append("| --- | ---: | ---: |")
    for ref in REFERENCES:
        lines.append(f"| `{ref}` | {old_refs[ref]} | {new_refs[ref]} |")
    lines.append("")
    lines.append("## Build-Shape Assessment")
    lines.append("")
    lines.append(f"- `apps/admin-android` is task-discovery ready: {'yes' if new_shape_ready else 'no'}")
    lines.append("- `android_admin/` remains fallback/reference.")
    lines.append("")
    lines.append("## Warnings")
    lines.append("")
    lines.append("- Do not delete `android_admin/`.")
    lines.append("- Do not make `apps/admin-android` the release root yet.")
    lines.append("- Do not run `assembleRelease`.")
    lines.append("- Do not sign APKs.")
    lines.append("- Do not upload APKs.")
    lines.append("")
    lines.append("## Notes")
    lines.append("")
    lines.append("- Gradle is not run by this manifest.")
    lines.append("- Excluded directories: `.git`, `node_modules`, `.wrangler`, `.gradle`, `build`, `dist`, `cache`.")
    lines.append("- Large and binary files are counted by path and size but are not scanned for references.")
    lines.append("- File contents and secret values are not printed.")
    lines.append("- This report is generated by `tools/release/aifred_admin_parity_manifest.py`.")
    lines.append("")
    return "\n".join(lines)


def normalize_report(text: str) -> str:
    normalized: list[str] = []
    for line in text.splitlines():
        if line.startswith("Timestamp: "):
            normalized.append("Timestamp: <normalized>")
        elif line.startswith("Git branch: "):
            normalized.append("Git branch: `<normalized>`")
        elif line.startswith("Git commit: "):
            normalized.append("Git commit: `<normalized>`")
        else:
            normalized.append(line)
    return "\n".join(normalized).rstrip() + "\n"


def write_report() -> None:
    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(build_report(), encoding="utf-8")


def check_report() -> int:
    if not REPORT_PATH.is_file():
        print(f"Missing report: {REPORT_PATH.relative_to(ROOT)}", file=sys.stderr)
        return 1
    expected = normalize_report(build_report())
    current = normalize_report(REPORT_PATH.read_text(encoding="utf-8"))
    if expected != current:
        print(f"Stale report: {REPORT_PATH.relative_to(ROOT)}", file=sys.stderr)
        print("Run: python3 tools/release/aifred_admin_parity_manifest.py", file=sys.stderr)
        return 1
    print(f"Report is current: {REPORT_PATH.relative_to(ROOT)}")
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="compare generated report content with the checked-in report")
    mode.add_argument("--stdout", action="store_true", help="print generated report content without writing")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    if args.check:
        raise SystemExit(check_report())
    if args.stdout:
        print(build_report(), end="")
    else:
        write_report()
        print(f"Wrote {REPORT_PATH.relative_to(ROOT)}")
