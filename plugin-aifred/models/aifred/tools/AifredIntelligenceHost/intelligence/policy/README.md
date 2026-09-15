# intelligence/policy

Purpose: define how AIFRED reasons from evidence and what it is allowed to claim.

This folder owns analysis/recommendation rules. Personality does not.

Minimum claim classes:
- **fact** — directly supported by current AIFRED-owned data;
- **supported inference** — reasonable interpretation with identified evidence;
- **possible explanation** — plausible but not established; wording must preserve uncertainty;
- **unknown** — unavailable or insufficient evidence.

Required policies:
- observation freshness/sufficiency;
- reference compatibility;
- claim/evidence provenance;
- uncertainty language;
- recommendation boundaries;
- positive/no-change outcomes;
- no fabricated thresholds/targets;
- no unsupported causal attribution;
- no DAW-state assumptions.

AIFRED must be able to say that the mix is behaving well and that no change is indicated. It must not manufacture problems to appear useful.

Policy is provider-independent and must be testable without evaluating writing style.
