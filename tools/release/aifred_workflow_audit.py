#!/usr/bin/env python3
"""Generate a read-only AIFRED Phase 3 workflow and deployment audit."""

from __future__ import annotations

import argparse
import datetime as _dt
import os
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REPORT_PATH = ROOT / "docs" / "operations" / "PHASE3_WORKFLOW_AUDIT.md"
GENERATED_REPORTS = {
    "docs/operations/PHASE2_REPO_INVENTORY.md",
    "docs/operations/PHASE3_WORKFLOW_AUDIT.md",
    "docs/operations/PHASE4_WEBSITE_DRYRUN_REPORT.md",
    "docs/operations/PHASE4_ADMIN_DRYRUN_REPORT.md",
    "docs/operations/PHASE8_WEBSITE_PARITY_MANIFEST.md",
    "docs/operations/PHASE8_ADMIN_PARITY_MANIFEST.md",
    "docs/operations/PHASE8_PREVIEW_GATE_REPORT.md",
}

SCAN_ROOTS = [
    ".github/workflows",
    "tools",
    "infra",
    "website",
    "apps/website",
    "android_admin",
    "apps/admin-android",
    "docs",
    "README.md",
]

EXCLUDED_DIRS = {
    ".git",
    "node_modules",
    ".wrangler",
    ".gradle",
    "build",
    "dist",
    "cache",
}

DEPLOYMENT_INDICATORS = [
    "wrangler",
    "pages deploy",
    "cloudflare",
    "CLOUDFLARE_API_TOKEN",
    "project-name",
    "north3rnlight3r",
    "aifred-site",
    "github release",
    "gh release",
    "upload-artifact",
    "download-artifact",
    "AIFRED_RELEASE_VERSION",
    "AIFRED_PLUGIN_RELEASE_TAG",
    "AIFRED_GITHUB_REPO",
    "package-aifred",
    "package-aifred-macos",
    "setup-aifred-local-ai",
    "AifredWindowsInstaller",
    "AifredWindowsUninstaller",
    "VST3",
    ".vst3",
    "msbuild",
    "cmake",
    "dotnet publish",
    "gradlew",
    "assembleRelease",
    "openai",
    "OPENAI_API_KEY",
    "PAYPAL",
    "R2",
    "AIFRED_REFERENCE_POOL",
]

PATH_INDICATORS = [
    "website/",
    "apps/website",
    "android_admin",
    "apps/admin-android",
    "plugin-aifred",
    "tools/AifredEngine",
]

DEPLOYMENT_COMMAND_WARNINGS = [
    "wrangler deploy",
    "pages deploy",
    "gh release",
    "upload-artifact",
    "download-artifact",
    "assembleRelease",
    "dotnet publish",
    "cmake",
    "msbuild",
]

SECRET_NAME_WARNINGS = [
    "CLOUDFLARE_API_TOKEN",
    "OPENAI_API_KEY",
    "PAYPAL",
    "PAYPAL_CLIENT_SECRET",
]

PREVIEW_FORBIDDEN_TOKENS = [
    "wrangler deploy",
    "pages deploy",
    "CLOUDFLARE_API_TOKEN",
    "CF_API_TOKEN",
    "gh release create",
    "git push",
    "upload-artifact",
    "actions/upload-artifact",
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


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def is_binary_or_large(path: Path) -> bool:
    try:
        if path.stat().st_size > 5 * 1024 * 1024:
            return True
        data = path.read_bytes()[:4096]
    except OSError:
        return True
    return b"\0" in data


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return ""


def iter_scan_files() -> list[Path]:
    files: list[Path] = []
    for root_name in SCAN_ROOTS:
        root = ROOT / root_name
        if not root.exists():
            continue
        if root.is_file():
            if rel(root) not in GENERATED_REPORTS and not is_binary_or_large(root):
                files.append(root)
            continue
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = [name for name in dirnames if name not in EXCLUDED_DIRS]
            base = Path(dirpath)
            for filename in filenames:
                path = base / filename
                if rel(path) in GENERATED_REPORTS or is_binary_or_large(path):
                    continue
                files.append(path)
    return sorted(set(files), key=lambda item: rel(item))


def find_indicators(text: str, indicators: list[str]) -> list[str]:
    lower_text = text.lower()
    found: list[str] = []
    for indicator in indicators:
        if indicator.lower() in lower_text:
            found.append(indicator)
    return found


def workflow_files() -> list[Path]:
    root = ROOT / ".github" / "workflows"
    if not root.is_dir():
        return []
    return sorted(
        [path for path in root.iterdir() if path.is_file() and path.suffix in {".yml", ".yaml"}],
        key=lambda item: item.name,
    )


def parse_workflow(path: Path) -> tuple[str, list[str]]:
    text = read_text(path)
    name = path.stem
    triggers: list[str] = []
    in_on = False
    on_indent = 0
    trigger_indent = 2
    for line in text.splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        indent = len(line) - len(line.lstrip(" "))
        name_match = re.match(r"^name:\s*(.+?)\s*$", line)
        if name_match:
            name = name_match.group(1).strip().strip("'\"")
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
    return name, triggers


def generated_at() -> str:
    return _dt.datetime.now().astimezone().isoformat(timespec="seconds")


def bullet_list(items: list[str]) -> list[str]:
    if not items:
        return ["- None found."]
    return [f"- {item}" for item in items]


def table_by_file(rows: dict[str, list[str]]) -> list[str]:
    lines = ["| File | References |", "| --- | --- |"]
    if not rows:
        lines.append("| None found | n/a |")
        return lines
    for path in sorted(rows):
        refs = ", ".join(f"`{item}`" for item in rows[path])
        lines.append(f"| `{path}` | {refs} |")
    return lines


def build_report() -> str:
    files = iter_scan_files()
    deployment_refs: dict[str, list[str]] = {}
    path_refs: dict[str, list[str]] = {}
    command_warnings: dict[str, list[str]] = {}
    secret_warnings: dict[str, list[str]] = {}

    for path in files:
        text = read_text(path)
        rel_path = rel(path)
        deployment_found = find_indicators(text, DEPLOYMENT_INDICATORS)
        path_found = find_indicators(text, PATH_INDICATORS)
        command_found = find_indicators(text, DEPLOYMENT_COMMAND_WARNINGS)
        secret_found = find_indicators(text, SECRET_NAME_WARNINGS)
        if deployment_found:
            deployment_refs[rel_path] = deployment_found
        if path_found:
            path_refs[rel_path] = path_found
        if command_found:
            command_warnings[rel_path] = command_found
        if secret_found:
            secret_warnings[rel_path] = secret_found

    workflows = workflow_files()
    workflow_rows: list[str] = []
    references_old_website = False
    references_new_website = False
    references_old_admin = False
    references_new_admin = False
    references_plugin = False
    references_engine = False
    old_website_deploy_root = False
    new_website_deploy_root = False
    release_publishing_behavior = False
    tag_triggered_release_behavior = False
    manual_only_validation_workflow = False
    manual_only_preview_workflow = False
    preview_workflow_uses_apps_website = False
    preview_workflow_uses_secrets = False
    preview_workflow_has_deploy_commands = False
    apps_website_preview_shape_exists = all(
        (ROOT / path).is_file()
        for path in [
            "apps/website/_worker.js",
            "apps/website/functions/api/[[path]].js",
            "apps/website/functions/api/v1/[[path]].js",
            "apps/website/functions/ws/chat.js",
            "apps/website/index.html",
        ]
    )
    phase7_authorization_docs_exist = all(
        (ROOT / path).is_file()
        for path in [
            "docs/operations/PHASE7_PREVIEW_AUTHORIZATION_PACKAGE.md",
            "docs/operations/PHASE7_PREVIEW_EVIDENCE_TEMPLATE.md",
            "docs/operations/PHASE7_PREVIEW_ABORT_CRITERIA.md",
            "docs/operations/PHASE7_PRODUCTION_PROMOTION_BLOCKER.md",
            "docs/operations/PHASE7_APPROVAL_RECORD_TEMPLATE.md",
        ]
    )
    phase8_website_manifest_exists = (ROOT / "docs/operations/PHASE8_WEBSITE_PARITY_MANIFEST.md").is_file()
    phase8_admin_manifest_exists = (ROOT / "docs/operations/PHASE8_ADMIN_PARITY_MANIFEST.md").is_file()
    phase8_preview_gate_exists = (ROOT / "docs/operations/PHASE8_PREVIEW_GATE_REPORT.md").is_file()
    phase8_production_promotion_blocked = (
        ROOT / "docs/operations/PHASE7_PRODUCTION_PROMOTION_BLOCKER.md"
    ).is_file()
    phase9_human_review_packet_exists = (
        ROOT / "docs/operations/PHASE9_HUMAN_PREVIEW_REVIEW_PACKET.md"
    ).is_file()
    phase9_blocker_closure_checklist_exists = (
        ROOT / "docs/operations/PHASE9_BLOCKER_CLOSURE_CHECKLIST.md"
    ).is_file()
    phase9_preview_evidence_readiness_exists = (
        ROOT / "docs/operations/PHASE9_PREVIEW_EVIDENCE_READINESS_CHECKLIST.md"
    ).is_file()
    phase9_final_non_approval_exists = (
        ROOT / "docs/operations/PHASE9_FINAL_NON_APPROVAL_STATEMENT.md"
    ).is_file()
    phase9_preview_readiness_closure_exists = (
        ROOT / "docs/operations/PHASE9_PREVIEW_READINESS_CLOSURE_REPORT.md"
    ).is_file()
    phase10_preview_execution_checklist_exists = (
        ROOT / "docs/operations/PHASE10_PREVIEW_EXECUTION_CHECKLIST_DRAFT.md"
    ).is_file()
    phase10_go_no_go_exists = (
        ROOT / "docs/operations/PHASE10_GO_NO_GO_CRITERIA.md"
    ).is_file()
    phase10_evidence_capture_table_exists = (
        ROOT / "docs/operations/PHASE10_PREVIEW_EVIDENCE_CAPTURE_TABLE.md"
    ).is_file()
    phase10_rollback_observation_exists = (
        ROOT / "docs/operations/PHASE10_ROLLBACK_OBSERVATION_PLAN.md"
    ).is_file()
    phase10_preview_evidence_readiness_exists = (
        ROOT / "docs/operations/PHASE10_PREVIEW_EVIDENCE_READINESS_REPORT.md"
    ).is_file()
    phase11_approval_intake_exists = (
        ROOT / "docs/operations/PHASE11_HUMAN_APPROVAL_INTAKE_FORM.md"
    ).is_file()
    phase11_decision_record_exists = (
        ROOT / "docs/operations/PHASE11_PREVIEW_AUTHORIZATION_DECISION_RECORD.md"
    ).is_file()
    phase11_blocker_summary_exists = (
        ROOT / "docs/operations/PHASE11_APPROVAL_BLOCKER_SUMMARY.md"
    ).is_file()
    phase11_future_approval_instructions_exists = (
        ROOT / "docs/operations/PHASE11_FUTURE_APPROVAL_INSTRUCTIONS.md"
    ).is_file()
    phase11_decision_closure_exists = (
        ROOT / "docs/operations/PHASE11_DECISION_CLOSURE_REPORT.md"
    ).is_file()

    for path in workflows:
        text = read_text(path)
        name, triggers = parse_workflow(path)
        workflow_rows.append(
            f"| `{rel(path)}` | {name} | {', '.join(triggers) if triggers else 'unknown'} |"
        )
        lower_text = text.lower()
        references_old_website = references_old_website or "website/" in lower_text or "pages deploy website" in lower_text
        references_new_website = references_new_website or "apps/website" in lower_text
        references_old_admin = references_old_admin or "android_admin" in lower_text
        references_new_admin = references_new_admin or "apps/admin-android" in lower_text
        references_plugin = references_plugin or "plugin-aifred" in lower_text
        references_engine = references_engine or "tools/aifredengine" in lower_text
        old_website_deploy_root = old_website_deploy_root or "pages deploy website" in lower_text
        new_website_deploy_root = new_website_deploy_root or "pages deploy apps/website" in lower_text
        release_publishing_behavior = release_publishing_behavior or "gh release" in lower_text
        tag_triggered_release_behavior = tag_triggered_release_behavior or "refs/tags" in lower_text or "tags:" in lower_text
        if path.name == "aifred-monorepo-validate.yml" and triggers == ["workflow_dispatch"]:
            manual_only_validation_workflow = True
        if path.name == "aifred-website-preview-dryrun.yml":
            manual_only_preview_workflow = triggers == ["workflow_dispatch"]
            preview_workflow_uses_apps_website = "apps/website" in lower_text
            preview_workflow_uses_secrets = "secrets." in lower_text or any(
                token.lower() in lower_text
                for token in ["CLOUDFLARE_API_TOKEN", "CF_API_TOKEN", "PAYPAL_CLIENT_SECRET", "OPENAI_API_KEY"]
            )
            preview_workflow_has_deploy_commands = any(
                token.lower() in lower_text
                for token in PREVIEW_FORBIDDEN_TOKENS
            )

    duplicate_authority_warnings: list[str] = []
    if references_old_website and references_new_website:
        duplicate_authority_warnings.append("Workflows reference both `website/` and `apps/website`.")
    elif references_old_website:
        duplicate_authority_warnings.append("Workflows currently reference old `website/` paths.")
    elif references_new_website:
        duplicate_authority_warnings.append("Workflows currently reference new `apps/website` paths.")
    else:
        duplicate_authority_warnings.append("No workflow website path reference detected.")

    if references_old_admin and references_new_admin:
        duplicate_authority_warnings.append("Workflows reference both `android_admin` and `apps/admin-android`.")
    elif references_old_admin:
        duplicate_authority_warnings.append("Workflows currently reference old `android_admin` paths.")
    elif references_new_admin:
        duplicate_authority_warnings.append("Workflows currently reference new `apps/admin-android` paths.")
    else:
        duplicate_authority_warnings.append("No workflow Android admin path reference detected.")

    if references_plugin:
        duplicate_authority_warnings.append("Workflows reference `plugin-aifred`.")
    if references_engine:
        duplicate_authority_warnings.append("Workflows reference `tools/AifredEngine`.")

    lines: list[str] = []
    lines.append("# Phase 3 Workflow Audit")
    lines.append("")
    lines.append(f"Timestamp: {generated_at()}")
    lines.append("")
    lines.append(f"Git branch: `{git_output('branch', '--show-current')}`")
    lines.append(f"Git commit: `{git_output('rev-parse', '--short', 'HEAD')}`")
    lines.append("")
    lines.append("## Workflows Found")
    lines.append("")
    lines.append("| File | Name | Triggers |")
    lines.append("| --- | --- | --- |")
    if workflow_rows:
        lines.extend(workflow_rows)
    else:
        lines.append("| None found | n/a | n/a |")
    lines.append("")
    lines.append("## Existing Workflow Path Usage")
    lines.append("")
    lines.append(f"- References old `website/`: {'yes' if references_old_website else 'no'}")
    lines.append(f"- References new `apps/website`: {'yes' if references_new_website else 'no'}")
    lines.append(f"- References old `android_admin`: {'yes' if references_old_admin else 'no'}")
    lines.append(f"- References new `apps/admin-android`: {'yes' if references_new_admin else 'no'}")
    lines.append(f"- References `plugin-aifred`: {'yes' if references_plugin else 'no'}")
    lines.append(f"- References `tools/AifredEngine`: {'yes' if references_engine else 'no'}")
    lines.append("")
    lines.append("## Deployment And Release Behavior Detection")
    lines.append("")
    lines.append(f"- Old website deployment root detected: {'yes' if old_website_deploy_root else 'no'}")
    lines.append(f"- New `apps/website` deployment root detected: {'yes' if new_website_deploy_root else 'no'}")
    lines.append(f"- Release publishing behavior detected: {'yes' if release_publishing_behavior else 'no'}")
    lines.append(f"- Tag-triggered release behavior detected: {'yes' if tag_triggered_release_behavior else 'no'}")
    lines.append(f"- Manual-only monorepo validation workflow detected: {'yes' if manual_only_validation_workflow else 'no'}")
    lines.append("")
    lines.append("## Preview Dry-Run Workflow Detection")
    lines.append("")
    lines.append(f"- Manual-only preview dry-run workflow detected: {'yes' if manual_only_preview_workflow else 'no'}")
    lines.append(f"- Preview workflow uses `apps/website`: {'yes' if preview_workflow_uses_apps_website else 'no'}")
    lines.append(f"- Preview workflow uses secrets: {'yes' if preview_workflow_uses_secrets else 'no'}")
    lines.append(f"- Preview workflow contains deploy/release/artifact commands: {'yes' if preview_workflow_has_deploy_commands else 'no'}")
    lines.append("")
    lines.append("## Phase 6 Gate Summary")
    lines.append("")
    lines.append(
        f"- Production workflow still unchanged: {'yes' if old_website_deploy_root and not new_website_deploy_root and release_publishing_behavior else 'no'}"
    )
    lines.append(f"- Preview dry-run workflow is manual-only: {'yes' if manual_only_preview_workflow else 'no'}")
    lines.append(f"- `apps/website` preview shape exists: {'yes' if apps_website_preview_shape_exists else 'no'}")
    lines.append("- Merge remains blocked by asset strategy and Cloudflare manual verification.")
    lines.append("")
    lines.append("## Phase 7 Authorization Summary")
    lines.append("")
    lines.append(f"- Preview authorization docs exist: {'yes' if phase7_authorization_docs_exist else 'no'}")
    lines.append("- Production promotion remains blocked.")
    lines.append(f"- Preview workflow remains manual-only: {'yes' if manual_only_preview_workflow else 'no'}")
    lines.append(
        f"- Production workflow remains unchanged: {'yes' if old_website_deploy_root and not new_website_deploy_root and release_publishing_behavior else 'no'}"
    )
    lines.append("- Merge remains blocked pending human approval and asset strategy.")
    lines.append("")
    lines.append("## Phase 8 Local Preview Harness Summary")
    lines.append("")
    lines.append(f"- Website parity manifest exists: {'yes' if phase8_website_manifest_exists else 'no'}")
    lines.append(f"- Admin parity manifest exists: {'yes' if phase8_admin_manifest_exists else 'no'}")
    lines.append(f"- Preview gate report exists: {'yes' if phase8_preview_gate_exists else 'no'}")
    lines.append(f"- Preview workflow remains manual-only: {'yes' if manual_only_preview_workflow else 'no'}")
    lines.append(
        f"- Production workflow remains unchanged: {'yes' if old_website_deploy_root and not new_website_deploy_root and release_publishing_behavior else 'no'}"
    )
    lines.append(f"- Production promotion remains blocked: {'yes' if phase8_production_promotion_blocked else 'no'}")
    lines.append("")
    lines.append("## Phase 9 Review Closure Summary")
    lines.append("")
    lines.append(f"- Human review packet exists: {'yes' if phase9_human_review_packet_exists else 'no'}")
    lines.append(f"- Blocker closure checklist exists: {'yes' if phase9_blocker_closure_checklist_exists else 'no'}")
    lines.append(f"- Preview evidence readiness checklist exists: {'yes' if phase9_preview_evidence_readiness_exists else 'no'}")
    lines.append(f"- Final non-approval statement exists: {'yes' if phase9_final_non_approval_exists else 'no'}")
    lines.append(f"- Preview readiness closure report exists: {'yes' if phase9_preview_readiness_closure_exists else 'no'}")
    lines.append(f"- Production promotion remains blocked: {'yes' if phase8_production_promotion_blocked else 'no'}")
    lines.append("- Preview remains not executed.")
    lines.append("")
    lines.append("## Phase 10 Evidence Planning Summary")
    lines.append("")
    lines.append(f"- Preview execution checklist draft exists: {'yes' if phase10_preview_execution_checklist_exists else 'no'}")
    lines.append(f"- Go/no-go criteria exists: {'yes' if phase10_go_no_go_exists else 'no'}")
    lines.append(f"- Evidence capture table exists: {'yes' if phase10_evidence_capture_table_exists else 'no'}")
    lines.append(f"- Rollback observation plan exists: {'yes' if phase10_rollback_observation_exists else 'no'}")
    lines.append(f"- Preview evidence readiness report exists: {'yes' if phase10_preview_evidence_readiness_exists else 'no'}")
    lines.append("- Preview remains not executed.")
    lines.append(
        f"- Production remains unchanged: {'yes' if old_website_deploy_root and not new_website_deploy_root and release_publishing_behavior else 'no'}"
    )
    lines.append(f"- Production promotion remains blocked: {'yes' if phase8_production_promotion_blocked else 'no'}")
    lines.append("")
    lines.append("## Phase 11 Approval Intake Summary")
    lines.append("")
    lines.append(f"- Approval intake form exists: {'yes' if phase11_approval_intake_exists else 'no'}")
    lines.append(f"- Decision record exists: {'yes' if phase11_decision_record_exists else 'no'}")
    lines.append(f"- Approval blocker summary exists: {'yes' if phase11_blocker_summary_exists else 'no'}")
    lines.append(f"- Future approval instructions exist: {'yes' if phase11_future_approval_instructions_exists else 'no'}")
    lines.append(f"- Decision closure report exists: {'yes' if phase11_decision_closure_exists else 'no'}")
    lines.append("- Preview remains not executed.")
    lines.append(
        f"- Production remains unchanged: {'yes' if old_website_deploy_root and not new_website_deploy_root and release_publishing_behavior else 'no'}"
    )
    lines.append("- Preview is not authorized in this phase.")
    lines.append(f"- Production promotion remains blocked: {'yes' if phase8_production_promotion_blocked else 'no'}")
    lines.append("")
    lines.append("## Deployment-Related References By File")
    lines.append("")
    lines.extend(table_by_file(deployment_refs))
    lines.append("")
    lines.append("## Path References By File")
    lines.append("")
    lines.extend(table_by_file(path_refs))
    lines.append("")
    lines.append("## Potential Duplicate Deployment Authorities")
    lines.append("")
    lines.extend(bullet_list(duplicate_authority_warnings))
    lines.append("")
    lines.append("## Deployment-Looking Command Warnings")
    lines.append("")
    lines.extend(table_by_file(command_warnings))
    lines.append("")
    lines.append("## Secret-Looking Variable Name Warnings")
    lines.append("")
    lines.extend(table_by_file(secret_warnings))
    lines.append("")
    lines.append("No secret values are printed in this report. Only variable names and path references are reported.")
    lines.append("")
    lines.append("## Phase 4 Recommendations")
    lines.append("")
    lines.append("- Keep existing live deployment workflow behavior unchanged until explicit path migration approval.")
    lines.append("- Prove `apps/website` with non-deploying syntax and route checks before changing Cloudflare Pages commands.")
    lines.append("- Verify Cloudflare Pages project bindings manually before any monorepo deployment migration.")
    lines.append("- Keep GitHub release publishing on existing package paths until plugin and engine migrations are separately proven.")
    lines.append("- Decide media asset strategy before merging the consolidation branch to `main`.")
    lines.append("")
    lines.append("## Notes")
    lines.append("")
    lines.append("- Excluded directories: `.git`, `node_modules`, `.wrangler`, `.gradle`, `build`, `dist`, `cache`.")
    lines.append("- File contents and secret values are not printed.")
    lines.append("- This report is generated by `tools/release/aifred_workflow_audit.py`.")
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
        print("Run: python3 tools/release/aifred_workflow_audit.py", file=sys.stderr)
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
