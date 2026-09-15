# intelligence/response

Purpose: turn validated evidence and provider reasoning into a structured response plan before conversational styling reaches the user.

A response plan should be able to represent:
- direct answer;
- relevant findings;
- evidence references;
- claim class/confidence;
- uncertainty/unknowns;
- recommended investigation areas;
- explicit “no change indicated” outcomes;
- unavailable capabilities/data.

The final conversational renderer/personality may rewrite phrasing but must not add claims that are absent from the validated plan.

Rules:
- every factual mix claim points to AIFRED-owned evidence;
- causal claims are never stronger than the available evidence;
- unsupported plugin/track/source attribution is forbidden;
- reference comparisons require compatible reference state;
- no response may instruct or trigger autonomous mix mutation.

This folder is the hallucination firewall between model output and user-visible guidance.
