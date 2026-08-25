#!/usr/bin/env python3
"""Generate a local AIFRED Phase 8 website parity manifest."""

from __future__ import annotations

import argparse
import datetime as _dt
import os
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REPORT_PATH = ROOT / "docs" / "operations" / "PHASE8_WEBSITE_PARITY_MANIFEST.md"

OLD_ROOT = ROOT / "website"
NEW_ROOT = ROOT / "apps" / "website"

CRITICAL_FILES = [
    "_worker.js",
    "functions/api/v1/[[path]].js",
    "functions/api/[[path]].js",
    "functions/ws/chat.js",
    "index.html",
    "app.js",
    "config.js",
    "styles.css",
    "assets/data/beat_catalog.json",
]

REFERENCES = [
    "/api/v1",
    "/api/",
    "/ws/chat",
    "downloads/plugin",
    "free",
    "download",
    "catalog",
    "inquiry",
    "admin",
    "activity",
    "R2",
    "GITHUB",
    "north3rnlight3r.com",
]

MEDIA_EXTENSIONS = {
    ".mp3",
    ".wav",
    ".aif",
    ".aiff",
    ".flac",
    ".ogg",
    ".m4a",
    ".jpg",
    ".jpeg",
    ".png",
    ".gif",
    ".webp",
    ".mp4",
    ".mov",
}

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
            if ref in {"R2", "GITHUB"}:
                counts[ref] += text.count(ref)
            else:
                counts[ref] += lower.count(ref.lower())
    return counts


def media_summary(files: list[Path]) -> list[tuple[str, int, int]]:
    summary: dict[str, list[int]] = defaultdict(lambda: [0, 0])
    for path in files:
        ext = path.suffix.lower()
        if ext not in MEDIA_EXTENSIONS:
            continue
        try:
            size = path.stat().st_size
        except OSError:
            size = 0
        summary[ext][0] += 1
        summary[ext][1] += size
    return sorted((ext, values[0], values[1]) for ext, values in summary.items())


def status_text(path: Path) -> str:
    return "yes" if path.is_file() else "no"


def file_size(path: Path) -> str:
    try:
        return format_bytes(path.stat().st_size)
    except OSError:
        return "n/a"


def append_path_list(lines: list[str], paths: list[str]) -> None:
    if not paths:
        lines.append("- None.")
        return
    for path in paths:
        lines.append(f"- `{path}`")


def append_media_table(lines: list[str], title: str, media: list[tuple[str, int, int]]) -> None:
    lines.append(f"### {title}")
    lines.append("")
    lines.append("| Extension | Files | Size |")
    lines.append("| --- | ---: | ---: |")
    if media:
        for ext, count, size in media:
            lines.append(f"| `{ext}` | {count} | {format_bytes(size)} |")
    else:
        lines.append("| n/a | 0 | 0 B |")
    lines.append("")


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
    old_media = media_summary(list(old_files.values()))
    new_media = media_summary(list(new_files.values()))
    largest = sorted(
        [(path.stat().st_size, "website", rel_to_root(path, OLD_ROOT)) for path in old_files.values()]
        + [(path.stat().st_size, "apps/website", rel_to_root(path, NEW_ROOT)) for path in new_files.values()],
        reverse=True,
    )[:25]
    critical_new_ready = all((NEW_ROOT / path).is_file() for path in CRITICAL_FILES)
    preview_shape_ready = NEW_ROOT.is_dir() and critical_new_ready

    lines: list[str] = []
    lines.append("# Phase 8 Website Parity Manifest")
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
    lines.append(f"- Total file count in `website/`: {len(old_files)}")
    lines.append(f"- Total file count in `apps/website/`: {len(new_files)}")
    lines.append(f"- Shared relative paths count: {len(shared_paths)}")
    lines.append("")
    lines.append("## Files Only In `website/`")
    lines.append("")
    append_path_list(lines, only_old)
    lines.append("")
    lines.append("## Files Only In `apps/website/`")
    lines.append("")
    append_path_list(lines, only_new)
    lines.append("")
    lines.append("## Critical Route File Comparison")
    lines.append("")
    lines.append("| Relative path | `website/` present | `apps/website/` present | Old size | New size |")
    lines.append("| --- | --- | --- | ---: | ---: |")
    for rel_path in CRITICAL_FILES:
        old_path = OLD_ROOT / rel_path
        new_path = NEW_ROOT / rel_path
        lines.append(
            f"| `{rel_path}` | {status_text(old_path)} | {status_text(new_path)} | {file_size(old_path)} | {file_size(new_path)} |"
        )
    lines.append("")
    lines.append("## Media Summary By Extension")
    lines.append("")
    append_media_table(lines, "`website/`", old_media)
    append_media_table(lines, "`apps/website/`", new_media)
    lines.append("## Largest 25 Files Across Both Roots")
    lines.append("")
    lines.append("| Size | Root | Relative path |")
    lines.append("| ---: | --- | --- |")
    if largest:
        for size, root_name, rel_path in largest:
            lines.append(f"| {format_bytes(size)} | `{root_name}/` | `{rel_path}` |")
    else:
        lines.append("| 0 B | n/a | n/a |")
    lines.append("")
    lines.append("## Route And Backend Reference Counts")
    lines.append("")
    lines.append("| Reference | `website/` | `apps/website/` |")
    lines.append("| --- | ---: | ---: |")
    for ref in REFERENCES:
        lines.append(f"| `{ref}` | {old_refs[ref]} | {new_refs[ref]} |")
    lines.append("")
    lines.append("## Parity Assessment")
    lines.append("")
    lines.append(f"- Critical files exist in `apps/website`: {'yes' if critical_new_ready else 'no'}")
    lines.append(f"- `apps/website` is preview-shape ready: {'yes' if preview_shape_ready else 'no'}")
    lines.append("- Differences are expected because `website/` is preserved as fallback and `apps/website/` is the imported canonical candidate.")
    lines.append("")
    lines.append("## Warnings")
    lines.append("")
    lines.append("- Do not delete `website/`.")
    lines.append("- Do not make `apps/website` the production root yet.")
    lines.append("- Do not deploy.")
    lines.append("- Asset duplication remains unresolved.")
    lines.append("")
    lines.append("## Notes")
    lines.append("")
    lines.append("- Excluded directories: `.git`, `node_modules`, `.wrangler`, `.gradle`, `build`, `dist`, `cache`.")
    lines.append("- Large and binary files are counted by path and size but are not scanned for references.")
    lines.append("- File contents and secret values are not printed.")
    lines.append("- This report is generated by `tools/release/aifred_website_parity_manifest.py`.")
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
        print("Run: python3 tools/release/aifred_website_parity_manifest.py", file=sys.stderr)
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
