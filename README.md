# AIFRED

AIFRED is a fresh Windows-first VST3 and website build for North3rnLight3r. The plugin source in this repo is new JUCE/C++ code, with local reference documents and images used only to shape the product direction, brand language, and asset choices.

## Layout

- `plugin-aifred/` contains the AIFRED VST3 source and mascot asset.
- `website/` contains the storefront, beat catalog assets, and Cloudflare Pages Functions.
- `android_admin/` contains the renamed AIFRED admin app configuration surface.
- `.github/workflows/build.yml` builds Windows, macOS, Ubuntu, and Arch Linux targets.

## Windows VST3 Build

Install Visual Studio with the Desktop development with C++ workload, CMake, Git, and Ninja. From a Visual Studio developer shell:

```powershell
cmake -S . -B build/aifred -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/aifred --parallel
./tools/package-aifred.ps1 -BuildRoot build/aifred -Platform windows
```

The package script writes `dist/AIFRED-VST3-windows.zip`. Extract it, run `install-aifred.ps1` from PowerShell, then rescan plugins in FL Studio.

## Website Backend

Cloudflare Pages should use `website/` as the project root. Secrets stay in Cloudflare environment variables:

- `AIFRED_CHAT_PROVIDER`: `openai` or `ollama`
- `OPENAI_API_KEY`
- `OPENAI_MODEL`
- `OLLAMA_BASE_URL`
- `OLLAMA_MODEL`
- `AIFRED_CONTACT_EMAIL`

Catalog metadata is read from `website/assets/data/beat_catalog.json`, and beat files are served from `website/assets/audio/catalog/`.

## Deployment

The GitHub Actions workflow builds Windows, macOS, Ubuntu, Arch Linux, and the Android admin APK. Pushing `main` runs validation and deploys `website/` to Cloudflare Pages when these GitHub secrets exist:

- `CLOUDFLARE_ACCOUNT_ID`
- `CLOUDFLARE_API_TOKEN`

Tagging a release such as `v0.1.0` publishes the installable packages to GitHub Releases. The website Buy AIFRED button sends buyers to PayPal and returns them to the latest release page configured in `website/config.js`.
