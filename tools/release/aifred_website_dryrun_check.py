#!/usr/bin/env python3
"""Generate a non-deploying AIFRED website path dry-run report."""

from __future__ import annotations

import argparse
import datetime as _dt
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REPORT_PATH = ROOT / "docs" / "operations" / "PHASE4_WEBSITE_DRYRUN_REPORT.md"

OLD_ROOT = ROOT / "website"
NEW_ROOT = ROOT / "apps" / "website"

REQUIRED_PAIRS = [
    ("Worker entry", "website/_worker.js", "apps/website/_worker.js"),
    ("API v1 route", "website/functions/api/v1/[[path]].js", "apps/website/functions/api/v1/[[path]].js"),
    ("Legacy API route", "website/functions/api/[[path]].js", "apps/website/functions/api/[[path]].js"),
    ("WebSocket chat route", "website/functions/ws/chat.js", "apps/website/functions/ws/chat.js"),
    ("Frontend entry", "website/index.html", "apps/website/index.html"),
]

NEW_EXPECTED_FILES = [
    "apps/website/app.js",
    "apps/website/config.js",
    "apps/website/styles.css",
    "apps/website/assets/data/beat_catalog.json",
]

REFERENCES = [
    "/api/v1",
    "/api/",
    "/ws/chat",
    "paypal",
    "download",
    "catalog",
    "inquiry",
    "admin",
    "activity",
    "R2",
    "GITHUB",
    "north3rnlight3r.com",
]

EXCLUDED_DIRS = {".git", "node_modules", ".wrangler", ".gradle", "build", "dist", "cache"}


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


def exists(path: str) -> bool:
    return (ROOT / path).exists()


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def is_text_scan_candidate(path: Path) -> bool:
    try:
        if path.stat().st_size > 5 * 1024 * 1024:
            return False
        data = path.read_bytes()[:4096]
    except OSError:
        return False
    return b"\0" not in data


def iter_files(root: Path) -> list[Path]:
    if not root.is_dir():
        return []
    files: list[Path] = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [name for name in dirnames if name not in EXCLUDED_DIRS]
        base = Path(dirpath)
        for filename in filenames:
            path = base / filename
            if is_text_scan_candidate(path):
                files.append(path)
    return sorted(files, key=rel)


def reference_counts(root: Path) -> dict[str, int]:
    counts = {ref: 0 for ref in REFERENCES}
    for path in iter_files(root):
        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        lower = text.lower()
        for ref in REFERENCES:
            if ref == "R2" or ref == "GITHUB":
                counts[ref] += text.count(ref)
            else:
                counts[ref] += lower.count(ref.lower())
    return counts


def count_files(root: Path) -> int:
    if not root.is_dir():
        return 0
    total = 0
    for _, dirnames, filenames in os.walk(root):
        dirnames[:] = [name for name in dirnames if name not in EXCLUDED_DIRS]
        total += len(filenames)
    return total


def build_report() -> str:
    old_counts = reference_counts(OLD_ROOT)
    new_counts = reference_counts(NEW_ROOT)

    missing_required: list[str] = []
    for _, old_path, new_path in REQUIRED_PAIRS:
        if not exists(old_path):
            missing_required.append(old_path)
        if not exists(new_path):
            missing_required.append(new_path)
    for path in NEW_EXPECTED_FILES:
        if not exists(path):
            missing_required.append(path)

    new_required_ok = all(exists(new_path) for _, _, new_path in REQUIRED_PAIRS) and all(
        exists(path) for path in NEW_EXPECTED_FILES
    )

    warnings: list[str] = []
    if count_files(OLD_ROOT) != count_files(NEW_ROOT):
        warnings.append(f"File counts differ: `website` has {count_files(OLD_ROOT)} files; `apps/website` has {count_files(NEW_ROOT)} files.")
    for ref in REFERENCES:
        if old_counts[ref] != new_counts[ref]:
            warnings.append(f"Reference count differs for `{ref}`: old={old_counts[ref]}, new={new_counts[ref]}.")
    if exists("infra/cloudflare/wrangler.toml"):
        warnings.append("`infra/cloudflare/wrangler.toml` exists and should be reviewed before any deployment migration.")

    appears_compatible = new_required_ok
    recommendation = (
        "`apps/website` is ready for a non-production preview test after manual Cloudflare binding verification."
        if appears_compatible
        else "`apps/website` is not ready for preview testing until missing required files are resolved."
    )

    lines: list[str] = []
    lines.append("# Phase 4 Website Dry-Run Report")
    lines.append("")
    lines.append(f"Timestamp: {generated_at()}")
    lines.append("")
    lines.append(f"Git branch: `{git_output('branch', '--show-current')}`")
    lines.append(f"Git commit: `{git_output('rev-parse', '--short', 'HEAD')}`")
    lines.append("")
    lines.append("## Required File Checklist")
    lines.append("")
    lines.append("| Area | Old `website/` | New `apps/website/` |")
    lines.append("| --- | --- | --- |")
    for label, old_path, new_path in REQUIRED_PAIRS:
        lines.append(f"| {label} | {'yes' if exists(old_path) else 'no'} `{old_path}` | {'yes' if exists(new_path) else 'no'} `{new_path}` |")
    lines.append("")
    lines.append("## New Website Expected Files")
    lines.append("")
    lines.append("| File | Present |")
    lines.append("| --- | --- |")
    for path in NEW_EXPECTED_FILES:
        lines.append(f"| `{path}` | {'yes' if exists(path) else 'no'} |")
    lines.append("")
    lines.append("## Cloudflare Support File")
    lines.append("")
    lines.append(f"- `infra/cloudflare/wrangler.toml`: {'present' if exists('infra/cloudflare/wrangler.toml') else 'not present'}")
    lines.append("")
    lines.append("## Route And Backend Reference Counts")
    lines.append("")
    lines.append("| Reference | `website/` | `apps/website/` |")
    lines.append("| --- | ---: | ---: |")
    for ref in REFERENCES:
        lines.append(f"| `{ref}` | {old_counts[ref]} | {new_counts[ref]} |")
    lines.append("")
    lines.append("## Major Missing Files")
    lines.append("")
    if missing_required:
        lines.extend(f"- `{path}`" for path in missing_required)
    else:
        lines.append("- None.")
    lines.append("")
    lines.append("## Deploy Shape Compatibility")
    lines.append("")
    lines.append(f"- `apps/website` appears deploy-shape-compatible: {'yes' if appears_compatible else 'no'}")
    lines.append(f"- Recommendation: {recommendation}")
    lines.append("")
    lines.append("## Difference Warnings")
    lines.append("")
    if warnings:
        lines.extend(f"- {warning}" for warning in warnings)
    else:
        lines.append("- No reference-count or required-file differences detected.")
    lines.append("")
    lines.append("## Safety Notes")
    lines.append("")
    lines.append("- This dry-run does not call Wrangler, Cloudflare APIs, GitHub releases, or deployment commands.")
    lines.append("- File contents and secret values are not printed.")
    lines.append("- This report is generated by `tools/release/aifred_website_dryrun_check.py`.")
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
        print("Run: python3 tools/release/aifred_website_dryrun_check.py", file=sys.stderr)
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
