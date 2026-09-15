# intelligence/prompts

Purpose: assemble provider-neutral prompts/messages from AIFRED-owned policy, personality, context, and tool descriptions.

Prompt files are not a substitute for runtime validation. A prompt saying “do not hallucinate” does not replace typed availability/evidence checks.

Assembly order should keep concerns distinct:
1. architecture/evidence policy;
2. current capability/tool contract;
3. personality/style;
4. bounded context packet;
5. current user question.

Rules:
- no hard-coded fabricated mix targets;
- no provider-specific secret material;
- no raw unbounded session dump;
- no hidden instructions that grant DAW mutation;
- prompt revisions are versioned and covered by provider-parity/evidence tests.

The model should receive only the context necessary for the current request.
