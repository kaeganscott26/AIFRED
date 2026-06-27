# Release Workflow Safety Checklist

Phase 4 does not change release publishing behavior.

Before any release workflow edits:

- [ ] Confirm existing tag release behavior is unchanged.
- [ ] Confirm existing plugin packaging path remains `plugin-aifred`.
- [ ] Confirm existing engine path remains `tools/AifredEngine`.
- [ ] Confirm existing installer scripts remain in place.
- [ ] Confirm current GitHub release tag and expected assets.
- [ ] Confirm Windows packaging script behavior separately.
- [ ] Confirm macOS packaging script behavior separately.
- [ ] Confirm Linux packaging script behavior separately.
- [ ] Confirm validation workflows do not upload artifacts.
- [ ] Confirm validation workflows do not publish releases.

Do not change release publishing until plugin and engine path migration is separately planned and approved.

Do not publish releases from validation workflows.
