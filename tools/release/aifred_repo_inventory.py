#!/usr/bin/env python3
"""Generate a read-only AIFRED Phase 2 repository inventory report."""

from __future__ import annotations

import datetime as _dt
import os
import subprocess
from collections import Counter, defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REPORT_PATH = ROOT / "docs" / "operations" / "PHASE2_REPO_INVENTORY.md"

MAJOR_AREAS = [
    "apps/website",
    "apps/admin-android",
    "plugin-aifred",
    "tools/AifredEngine",
    "website",
    "android_admin",
    "infra/cloudflare",
    "packages",
    "docs",
]

BACKEND_REFERENCES = [
    "north3rnlight3r.com",
    "/api/v1",
    "127.0.0.1:8787",
    "127.0.0.1:11434",
    "aifred:latest",
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


def iter_files() -> list[Path]:
    files: list[Path] = []
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [name for name in dirnames if name != ".git"]
        base = Path(dirpath)
        for filename in filenames:
            files.append(base / filename)
    return files


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def format_bytes(size: int) -> str:
    value = float(size)
    for unit in ("B", "KB", "MB", "GB"):
        if value < 1024.0 or unit == "GB":
            if unit == "B":
                return f"{int(value)} {unit}"
            return f"{value:.1f} {unit}"
        value /= 1024.0
    return f"{size} B"


def top_level_summary() -> list[str]:
    rows: list[str] = []
    for entry in sorted(ROOT.iterdir(), key=lambda item: item.name.lower()):
        if entry.name == ".git":
            continue
        if entry.is_dir():
            direct_files = sum(1 for child in entry.iterdir() if child.is_file())
            direct_dirs = sum(1 for child in entry.iterdir() if child.is_dir())
            rows.append(f"- `{entry.name}/` - {direct_dirs} dirs, {direct_files} files at top level")
        else:
            rows.append(f"- `{entry.name}` - file, {format_bytes(entry.stat().st_size)}")
    return rows


def count_files_under(files: list[Path], area: str) -> tuple[int, int]:
    prefix = area.rstrip("/") + "/"
    count = 0
    size = 0
    for path in files:
        path_rel = rel(path)
        if path_rel == area or path_rel.startswith(prefix):
            count += 1
            try:
                size += path.stat().st_size
            except OSError:
                pass
    return count, size


def reference_counts(files: list[Path]) -> dict[str, int]:
    counts = {key: 0 for key in BACKEND_REFERENCES}
    for path in files:
        try:
            if path.stat().st_size > 5 * 1024 * 1024:
                continue
            data = path.read_bytes()
        except OSError:
            continue
        if b"\0" in data[:4096]:
            continue
        text = data.decode("utf-8", errors="ignore")
        for key in BACKEND_REFERENCES:
            counts[key] += text.count(key)
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


def write_report() -> None:
    files = iter_files()
    largest = sorted(
        ((path.stat().st_size, path) for path in files if path.exists()),
        reverse=True,
    )[:25]
    refs = reference_counts(files)
    media = media_summary(files)

    lines: list[str] = []
    lines.append("# Phase 2 Repository Inventory")
    lines.append("")
    lines.append(f"Timestamp: {_dt.datetime.now().astimezone().isoformat(timespec='seconds')}")
    lines.append("")
    lines.append(f"Git branch: `{git_output('branch', '--show-current')}`")
    lines.append(f"Git commit: `{git_output('rev-parse', '--short', 'HEAD')}`")
    lines.append("")
    lines.append("## Top-Level Directory Summary")
    lines.append("")
    lines.extend(top_level_summary())
    lines.append("")
    lines.append("## File Counts By Major Area")
    lines.append("")
    lines.append("| Area | Files | Size |")
    lines.append("| --- | ---: | ---: |")
    for area in MAJOR_AREAS:
        count, size = count_files_under(files, area)
        lines.append(f"| `{area}` | {count} | {format_bytes(size)} |")
    lines.append("")
    lines.append("## Largest 25 Files")
    lines.append("")
    lines.append("| Size | Path |")
    lines.append("| ---: | --- |")
    for size, path in largest:
        lines.append(f"| {format_bytes(size)} | `{rel(path)}` |")
    lines.append("")
    lines.append("## Media File Summary")
    lines.append("")
    lines.append("| Extension | Files | Size |")
    lines.append("| --- | ---: | ---: |")
    if media:
        for ext, count, size in media:
            lines.append(f"| `{ext}` | {count} | {format_bytes(size)} |")
    else:
        lines.append("| n/a | 0 | 0 B |")
    lines.append("")
    lines.append("## Duplicated Authority Warning")
    lines.append("")
    lines.append(f"- `website/` and `apps/website/` both exist: {'yes' if (ROOT / 'website').is_dir() and (ROOT / 'apps/website').is_dir() else 'no'}")
    lines.append(f"- `android_admin/` and `apps/admin-android/` both exist: {'yes' if (ROOT / 'android_admin').is_dir() and (ROOT / 'apps/admin-android').is_dir() else 'no'}")
    lines.append("- This coexistence is expected during Phase 2 and must be resolved only in a later approved migration phase.")
    lines.append("")
    lines.append("## Backend Reference Counts")
    lines.append("")
    lines.append("| Reference | Count |")
    lines.append("| --- | ---: |")
    for key in BACKEND_REFERENCES:
        lines.append(f"| `{key}` | {refs[key]} |")
    lines.append("")
    lines.append("## Notes")
    lines.append("")
    lines.append("- `.git` directories were excluded from this scan.")
    lines.append("- File contents and secret values are not printed.")
    lines.append("- This report is generated by `tools/release/aifred_repo_inventory.py`.")
    lines.append("")

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    write_report()
    print(f"Wrote {REPORT_PATH.relative_to(ROOT)}")
