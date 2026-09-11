# intelligence/session

Purpose: own current-session lifecycle, event reduction, comparison anchors, and session summaries.

Session intelligence begins downstream of `FilteredMixContext`. It must not duplicate BufferHunter statistics or store every high-frequency DSP publication.

Expected responsibilities:
- assign/validate session identity;
- detect session start/end/continuation;
- reduce meaningful context changes into `SessionEvent` records;
- retain question/advice/user-decision anchors;
- compare valid states across explicit anchors;
- compact a completed session into a bounded summary for `memory/`.

Event examples:
- observation became sufficient;
- profile/reference/epoch changed;
- sustained metric state changed materially;
- user asked a question;
- AIFRED returned an evidence-bound answer;
- user recorded a decision/intent.

Rules:
- no raw 10 Hz telemetry archive;
- no inference promoted to fact;
- session continuity never crosses incompatible project/profile/measurement identity silently;
- completed session summaries are deterministic inputs to memory rotation.
