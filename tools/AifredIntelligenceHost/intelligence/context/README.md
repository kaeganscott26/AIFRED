# intelligence/context

Purpose: assemble the smallest relevant, valid context packet for each intelligence request.

The context builder does not own measurement or long-term memory. It retrieves from authoritative sources and applies identity/freshness/budget rules.

Inputs may include:
- current `FilteredMixContext`;
- current session state;
- relevant retained session summaries;
- current reference state;
- current profile/configuration identity;
- read-only DAW context when Phase 3 is implemented;
- bounded conversation continuity;
- policy/personality references.

Rules:
- relevance beats volume;
- stale/incompatible epochs are excluded;
- no entire-session dump by default;
- no previous-session data unless the question benefits from it;
- every included fact keeps provenance/evidence identity;
- packet size is bounded and deterministic enough to test;
- provider token limits never become permission to drop required safety/evidence policy.

This folder owns context selection and packing, not semantic invention.
