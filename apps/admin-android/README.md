# AIFRED Admin | Mobile Management Console

Professional Android application for the internal management of the AIFRED web ecosystem and production catalog.

This application provides the project owner with forensic-level control over website operations, automated inquiry tracking, and catalog distribution, ensuring 100% operational uptime for the AIFRED platform.

## Management Features

- **Dynamic Catalog Control:** Full CRUD operations for the production beat catalog, including metadata management and file synchronization.
- **System Orchestration:** Website file control, asset deployment, and release documentation management.
- **Operational Command:** Real-time command dispatch for backend maintenance and API route monitoring.
- **Engineering Integration:** Advanced browser-analyzer summaries and DSP metric auditing.
- **AI Sync:** Integrated Ollama-aware model selection with professional fallback logic.

## Technical Specifications

- **Target Backend:** `https://www.north3rnlight3r.com`
- **Build System:** Gradle (Kotlin DSL)
- **Deployment:** Direct sideload or Google Play Internal Testing.

## Release Information

This admin app build is synchronized with the latest AIFRED platform update:
- **Plugin Version:** `0.3.3 Ollama chat polish`
- **Admin Version:** `2.4.2`
- **Release Tag:** `v0.3.3-ollama-chat`

## Local Development

```powershell
# Build Release APK
.\gradlew.bat assembleRelease

# Deploy to Device
adb install -r app\build\outputs\apk\release\app-release.apk

# Install Windows desktop companion
powershell -NoProfile -ExecutionPolicy Bypass -File tools\windows-admin\Install-AIFRED-Admin-Desktop.ps1
```

## Security

The admin app utilizes a secure, offline-aware login system. Access is restricted to the North3rnLight3r administrative credentials.
