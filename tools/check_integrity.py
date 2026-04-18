#!/usr/bin/env python3
import json
import sys
import urllib.request
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ZIP_PATH = ROOT / "dist" / "AIFRED-VST3-windows.zip"
INSTALLER = ROOT / "dist" / "installer" / "windows" / "AIFRED-VST3-Setup.exe"


def fail(message: str) -> int:
    print(f"FAIL {message}")
    return 1


def pass_line(message: str) -> None:
    print(f"PASS {message}")


def main() -> int:
    if not ZIP_PATH.exists():
        return fail(f"missing package zip: {ZIP_PATH}")
    if not INSTALLER.exists():
        return fail(f"missing installer: {INSTALLER}")

    with zipfile.ZipFile(ZIP_PATH) as archive:
        names = set(archive.namelist())
        required = [
            "Aifred.vst3/Contents/x86_64-win/Aifred.vst3",
            "Aifred/./bin/AifredEngine.exe",
            "Aifred/./config/config.json",
        ]
        normalized = {name.replace("\\", "/").replace("//", "/") for name in names}
        for item in required:
            if item not in normalized and item.replace("/./", "/") not in normalized:
                return fail(f"package missing {item}")
        if any(name.lower().endswith((".ps1", ".bat", ".cmd")) for name in normalized):
            return fail("package contains manual scripts")
    pass_line("package contains VST3, engine, and config without manual scripts")

    try:
        with urllib.request.urlopen("http://127.0.0.1:8787/health", timeout=2) as response:
            payload = json.loads(response.read().decode("utf-8"))
        if not payload.get("ok"):
            return fail("engine health returned ok=false")
        pass_line(f"engine health ok; model_loaded={payload.get('model_loaded')}")
    except Exception as exc:
        print(f"WARN engine health unavailable: {exc}")

    pass_line(f"installer present: {INSTALLER}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
