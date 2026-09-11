# intelligence/daw

Purpose: normalize whatever read-only session/DAW metadata a host can legitimately expose.

This folder does **not** assume every DAW exposes the same information. Each adapter declares capabilities and unavailable fields explicitly, then maps available data into AIFRED-owned contracts.

Potential read-only capabilities:
- host/project identity;
- transport/section metadata;
- track and mixer labels/IDs;
- buses/routes;
- plugin inventory;
- automation/parameter metadata when the host exposes it safely.

Permanent no-mutation rule:
AIFRED must never change audio, gain, faders, mute/solo/bypass state, routing, automation, plugin parameters, plugin inventory, or project state.

Search/inspection/navigation may be implemented where supported; inability to inspect something is an `unavailable` capability, not a reason to guess.

DAW adapters are Phase 3 work. Do not implement them as a prerequisite for Phase 1 or Phase 2.
