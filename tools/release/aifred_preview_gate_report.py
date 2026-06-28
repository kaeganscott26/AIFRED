#!/usr/bin/env python3
"""Generate a local AIFRED Phase 8 preview gate report."""

from __future__ import annotations

import argparse
import datetime as _dt
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REPORT_PATH = ROOT / "docs" / "operations" / "PHASE8_PREVIEW_GATE_REPORT.md"

PHASE7_DOCS = [
    "docs/operations/PHASE7_PREVIEW_AUTHORIZATION_PACKAGE.md",
    "docs/operations/PHASE7_PREVIEW_EVIDENCE_TEMPLATE.md",
    "docs/operations/PHASE7_PREVIEW_ABORT_CRITERIA.md",
    "docs/operations/PHASE7_PRODUCTION_PROMOTION_BLOCKER.md",
    "docs/operations/PHASE7_APPROVAL_RECORD_TEMPLATE.md",
]

PHASE6_DOCS = [
    "docs/operations/PHASE6_PREVIEW_APPROVAL_CHECKLIST.md",
    "docs/operations/PHASE6_MERGE_BLOCKER_REPORT.md",
    "docs/operations/PHASE6_PRODUCTION_NON_CHANGE_STATEMENT.md",
    "docs/operations/PHASE6_PREVIEW_RUNBOOK_DRAFT.md",
    "docs/operations/PHASE6_ASSET_ACCEPTANCE_CHECKLIST.md",
]

REQUIRED_PHASE8_ITEMS = [
    "tools/release/aifred_website_parity_manifest.py",
    "tools/release/aifred_admin_parity_manifest.py",
    "tools/release/aifred_preview_gate_report.py",
    "docs/operations/PHASE8_WEBSITE_PARITY_MANIFEST.md",
    "docs/operations/PHASE8_ADMIN_PARITY_MANIFEST.md",
    "docs/operations/PHASE8_PREVIEW_GATE_REPORT.md",
    "docs/operations/PHASE8_LOCAL_PREVIEW_PREFLIGHT.md",
]

PREVIEW_WORKFLOW = ".github/workflows/aifred-website-preview-dryrun.yml"
PREVIEW_FORBIDDEN_TOKENS = [
    "wrangler deploy",
    "pages deploy",
    "CLOUDFLARE_API_TOKEN",
    "CF_API_TOKEN",
    "gh release create",
    "gh release upload",
    "git push",
    "upload-artifact",
    "actions/upload-artifact",
    "download-artifact",
    "actions/download-artifact",
    "assembleRelease",
    "PAYPAL_CLIENT_SECRET",
    "OPENAI_API_KEY",
]


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


def read_text(path: str) -> str:
    try:
        return (ROOT / path).read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return ""


def parse_workflow_triggers(path: str) -> list[str]:
    text = read_text(path)
    triggers: list[str] = []
    in_on = False
    on_indent = 0
    trigger_indent = 2
    for line in text.splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        indent = len(line) - len(line.lstrip(" "))
        on_match = re.match(r"^on:\s*(.*)$", line)
        if on_match:
            in_on = True
            on_indent = indent
            trigger_indent = on_indent + 2
            inline = on_match.group(1).strip()
            if inline.startswith("[") and inline.endswith("]"):
                triggers.extend([part.strip().strip("'\"") for part in inline[1:-1].split(",") if part.strip()])
            continue
        if in_on:
            if indent <= on_indent:
                in_on = False
                continue
            if indent != trigger_indent:
                continue
            trigger_match = re.match(r"^\s+([A-Za-z0-9_-]+):", line)
            if trigger_match:
                trigger = trigger_match.group(1)
                if trigger not in triggers:
                    triggers.append(trigger)
    return triggers


def yes_no(value: bool) -> str:
    return "yes" if value else "no"


def build_report() -> str:
    preview_text = read_text(PREVIEW_WORKFLOW)
    preview_lower = preview_text.lower()
    preview_triggers = parse_workflow_triggers(PREVIEW_WORKFLOW)
    phase7_docs_exist = all(exists(path) for path in PHASE7_DOCS)
    phase6_docs_exist = all(exists(path) for path in PHASE6_DOCS)
    phase5_preview_workflow_exists = exists(PREVIEW_WORKFLOW)
    preview_manual_only = preview_triggers == ["workflow_dispatch"]
    preview_avoids_secrets = "secrets." not in preview_lower and not any(
        token.lower() in preview_lower
        for token in ["CLOUDFLARE_API_TOKEN", "CF_API_TOKEN", "PAYPAL_CLIENT_SECRET", "OPENAI_API_KEY"]
    )
    preview_avoids_forbidden = not any(token.lower() in preview_lower for token in PREVIEW_FORBIDDEN_TOKENS)
    production_build_exists = exists(".github/workflows/build.yml")
    old_website_exists = exists("website")
    new_website_exists = exists("apps/website")
    old_admin_exists = exists("android_admin")
    new_admin_exists = exists("apps/admin-android")
    plugin_exists = exists("plugin-aifred")
    engine_exists = exists("tools/AifredEngine")
    website_manifest_exists = exists("docs/operations/PHASE8_WEBSITE_PARITY_MANIFEST.md")
    admin_manifest_exists = exists("docs/operations/PHASE8_ADMIN_PARITY_MANIFEST.md")
    asset_strategy_unresolved = exists("docs/operations/PHASE5_ASSET_DECISION_RECORD.md") and exists(
        "docs/operations/PHASE6_ASSET_ACCEPTANCE_CHECKLIST.md"
    )
    cloudflare_required = exists("docs/operations/CLOUDFLARE_MANUAL_VERIFICATION_CHECKLIST.md")
    production_blocked = exists("docs/operations/PHASE7_PRODUCTION_PROMOTION_BLOCKER.md")
    required_phase8_exists = all(exists(path) for path in REQUIRED_PHASE8_ITEMS)
    safe_preview_workflow = (
        phase5_preview_workflow_exists
        and preview_manual_only
        and preview_avoids_secrets
        and preview_avoids_forbidden
    )
    ready = (
        phase7_docs_exist
        and phase6_docs_exist
        and required_phase8_exists
        and safe_preview_workflow
        and production_build_exists
        and old_website_exists
        and new_website_exists
        and old_admin_exists
        and new_admin_exists
        and plugin_exists
        and engine_exists
        and production_blocked
    )
    assessment = "READY_FOR_HUMAN_PREVIEW_REVIEW" if ready else "NOT_READY"

    checks = [
        ("Phase 7 authorization docs exist", phase7_docs_exist),
        ("Phase 6 gate docs exist", phase6_docs_exist),
        ("Phase 5 preview workflow exists", phase5_preview_workflow_exists),
        ("Preview workflow is manual-only", preview_manual_only),
        ("Preview workflow avoids secrets", preview_avoids_secrets),
        ("Preview workflow avoids deploy/release/artifact commands", preview_avoids_forbidden),
        ("Production `build.yml` still exists", production_build_exists),
        ("Old `website/` still exists", old_website_exists),
        ("New `apps/website/` exists", new_website_exists),
        ("Old `android_admin/` still exists", old_admin_exists),
        ("New `apps/admin-android/` exists", new_admin_exists),
        ("`plugin-aifred/` exists", plugin_exists),
        ("`tools/AifredEngine/` exists", engine_exists),
        ("Website parity manifest exists", website_manifest_exists),
        ("Admin parity manifest exists", admin_manifest_exists),
        ("Asset strategy remains unresolved", asset_strategy_unresolved),
        ("Cloudflare manual verification remains required", cloudflare_required),
        ("Production promotion remains blocked", production_blocked),
    ]

    lines: list[str] = []
    lines.append("# Phase 8 Preview Gate Report")
    lines.append("")
    lines.append(f"Timestamp: {generated_at()}")
    lines.append("")
    lines.append(f"Git branch: `{git_output('branch', '--show-current')}`")
    lines.append(f"Git commit: `{git_output('rev-parse', '--short', 'HEAD')}`")
    lines.append("")
    lines.append("## Gate Checks")
    lines.append("")
    lines.append("| Check | Result |")
    lines.append("| --- | --- |")
    for label, value in checks:
        lines.append(f"| {label} | {yes_no(value)} |")
    lines.append("")
    lines.append("## Final Gate Assessment")
    lines.append("")
    lines.append(f"- Assessment: `{assessment}`")
    lines.append("- Phase 8 does not authorize deployment.")
    lines.append("- Phase 8 does not authorize production.")
    lines.append("- Human approval is still required before any Cloudflare preview.")
    lines.append("- Asset strategy and Cloudflare verification remain blockers.")
    lines.append("")
    lines.append("## Notes")
    lines.append("")
    lines.append("- This report checks local files and workflow text only.")
    lines.append("- No Cloudflare, Gradle, deployment, release, artifact, or secret command is run.")
    lines.append("- This report is generated by `tools/release/aifred_preview_gate_report.py`.")
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
        print("Run: python3 tools/release/aifred_preview_gate_report.py", file=sys.stderr)
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
