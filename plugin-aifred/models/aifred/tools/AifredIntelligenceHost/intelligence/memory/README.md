# intelligence/memory

Purpose: retain a bounded active memory of the current session plus the previous nine session summaries.

This is not model memory and not an unbounded chat transcript store.

Hard contract:

```text
active memory = current session + previous 9 retained sessions
maximum active session count = 10
```

When an eleventh session enters active memory, the oldest retained session is evicted from active reasoning memory.

Responsibilities:
- store/retrieve bounded session summaries;
- preserve project/session/profile identities;
- enforce deterministic ordering and eviction;
- distinguish active memory from user-owned exports/archives;
- expose only relevant retained summaries to `context/`.

Rules:
- archived/exported sessions do not silently re-enter active memory;
- no provider chooses retention policy;
- no embedding/vector index may override the ten-session bound;
- no stale or incompatible session summary is treated as current evidence;
- privacy and deletion semantics must remain explicit and testable.

Current-session detail lives in `session/`; long-term active retention belongs here.
