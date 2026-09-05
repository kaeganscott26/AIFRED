# Testing

Run from the repository root:

```sh
node tools/generate-admin-command-reference.mjs --check
node --test tests/aifred-api.test.mjs tests/aifred-archive.test.mjs
npm --prefix apps run website:check
python -B scripts/common/check_repository.py
bash tools/release/aifred_monorepo_validate.sh
python -B tools/release/aifred_workflow_audit.py --check
```

Windows: scripts/windows/build.ps1 -Action test also builds the native plugin and runs tools/check-aifred-analysis-regressions.ps1. The PowerShell regression guard checks source contracts; it is not a DSP numerical standards suite. The current CMake project has no registered CTest DSP executable. Do not invent passing DSP tests.

After release assembly, run python -B scripts/common/release.py verify --platform windows-x64. tools/check_integrity.py checks the current ZIP and installer; live engine health is opt-in. API/archive tests use fixtures and do not deploy Cloudflare.

Manual release gates: FL Studio scanning/load, actual audio response, mono/stereo/clip behavior, no-signal UI, reference loading, chat availability/cancel, engine startup and settings, install/uninstall, reboot/login behavior and both-channel session compatibility. Windows compilation does not establish those results. macOS/Linux native and Android/admin builds are NOT VALIDATED by this pass.

Workflow checks preserve explicit/manual validation and existing release/deploy triggers. Report generators write generated output outside canonical docs. No checker should require a historical phase document to assert a current product fact.
