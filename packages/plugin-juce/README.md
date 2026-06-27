# AIFRED JUCE Plugin Package

Phase 1 consolidation placeholder.

The current canonical plugin source remains:

```text
../../plugin-aifred
```

Do not move or rewrite the runtime plugin source during Phase 1. The existing CMake files, packaging scripts, and release workflow still expect the plugin at the original path.

This package folder exists so the monorepo structure is visible without breaking current builds. A later phase can move the source here only after CMake, packaging, installer, and smoke tests are updated together.
