#!/usr/bin/env python3
"""Generate a non-building AIFRED Android admin path dry-run report."""

from __future__ import annotations

import argparse
import datetime as _dt
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REPORT_PATH = ROOT / "docs" / "operations" / "PHASE4_ADMIN_DRYRUN_REPORT.md"

OLD_ROOT = ROOT / "android_admin"
NEW_ROOT = ROOT / "apps" / "admin-android"

NEW_REQUIRED_FILES = [
    "apps/admin-android/settings.gradle.kts",
    "apps/admin-android/build.gradle.kts",
    "apps/admin-android/app/build.gradle.kts",
    "apps/admin-android/gradlew",
    "apps/admin-android/app/src/main/AndroidManifest.xml",
]

OLD_EQUIVALENT_FILES = [
    "android_admin/settings.gradle.kts",
    "android_admin/build.gradle.kts",
    "android_admin/app/build.gradle.kts",
    "android_admin/gradlew",
    "android_admin/app/src/main/AndroidManifest.xml",
]

REFERENCES = [
    "north3rnlight3r.com",
    "/api/v1",
    "/api/v1/admin",
    "login",
    "Bearer",
    "gradle",
    "versionName",
    "versionCode",
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
            if ref in {"Bearer", "versionName", "versionCode"}:
                counts[ref] += text.count(ref)
            else:
                counts[ref] += lower.count(ref.lower())
    return counts


def has_source_tree(root: Path) -> bool:
    main = root / "app" / "src" / "main"
    return (main / "java").is_dir() or (main / "kotlin").is_dir()


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

    missing_required = [path for path in NEW_REQUIRED_FILES if not exists(path)]
    if not has_source_tree(NEW_ROOT):
        missing_required.append("apps/admin-android/app/src/main/java or apps/admin-android/app/src/main/kotlin")

    new_required_ok = not missing_required and NEW_ROOT.is_dir()
    old_root_ok = OLD_ROOT.is_dir()

    warnings: list[str] = []
    if count_files(OLD_ROOT) != count_files(NEW_ROOT):
        warnings.append(f"File counts differ: `android_admin` has {count_files(OLD_ROOT)} files; `apps/admin-android` has {count_files(NEW_ROOT)} files.")
    for ref in REFERENCES:
        if old_counts[ref] != new_counts[ref]:
            warnings.append(f"Reference count differs for `{ref}`: old={old_counts[ref]}, new={new_counts[ref]}.")

    recommendation = (
        "`apps/admin-android` is ready for non-release Gradle task discovery when the local Android/Java environment is available."
        if new_required_ok
        else "`apps/admin-android` is not ready for Gradle task discovery until missing required files are resolved."
    )

    lines: list[str] = []
    lines.append("# Phase 4 Android Admin Dry-Run Report")
    lines.append("")
    lines.append(f"Timestamp: {generated_at()}")
    lines.append("")
    lines.append(f"Git branch: `{git_output('branch', '--show-current')}`")
    lines.append(f"Git commit: `{git_output('rev-parse', '--short', 'HEAD')}`")
    lines.append("")
    lines.append("## Required File Checklist")
    lines.append("")
    lines.append(f"- `android_admin` exists: {'yes' if old_root_ok else 'no'}")
    lines.append(f"- `apps/admin-android` exists: {'yes' if NEW_ROOT.is_dir() else 'no'}")
    lines.append(f"- `apps/admin-android` Java/Kotlin source tree exists: {'yes' if has_source_tree(NEW_ROOT) else 'no'}")
    lines.append("")
    lines.append("| New admin file | Present |")
    lines.append("| --- | --- |")
    for path in NEW_REQUIRED_FILES:
        lines.append(f"| `{path}` | {'yes' if exists(path) else 'no'} |")
    lines.append("")
    lines.append("| Old admin equivalent | Present |")
    lines.append("| --- | --- |")
    for path in OLD_EQUIVALENT_FILES:
        lines.append(f"| `{path}` | {'yes' if exists(path) else 'no'} |")
    lines.append("")
    lines.append("## Admin Reference Counts")
    lines.append("")
    lines.append("| Reference | `android_admin/` | `apps/admin-android/` |")
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
    lines.append("## Build Shape Compatibility")
    lines.append("")
    lines.append(f"- `apps/admin-android` appears build-shape-compatible: {'yes' if new_required_ok else 'no'}")
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
    lines.append("- This dry-run does not run Gradle, build APKs, sign APKs, publish releases, or upload artifacts.")
    lines.append("- File contents and secret values are not printed.")
    lines.append("- This report is generated by `tools/release/aifred_admin_dryrun_check.py`.")
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
        print("Run: python3 tools/release/aifred_admin_dryrun_check.py", file=sys.stderr)
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
