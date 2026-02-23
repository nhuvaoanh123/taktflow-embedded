---
paths:
  - "firmware/src/**/*.c"
  - "firmware/src/**/*.cpp"
---

## Human-in-the-Loop (HITL) Comment Lock

`HITL` means human-reviewer-owned comment content.

**Marker standard (code-friendly):**
- Markdown: `<!-- HITL-LOCK START:<id> -->` ... `<!-- HITL-LOCK END:<id> -->`
- C/C++/Java/JS/TS: `// HITL-LOCK START:<id>` ... `// HITL-LOCK END:<id>`
- Python/Shell/YAML/TOML: `# HITL-LOCK START:<id>` ... `# HITL-LOCK END:<id>`

**Rules:**
- AI must never edit, reformat, move, or delete text inside any `HITL-LOCK` block.
- Append-only: AI may add new comments/changes only; prior HITL comments stay unchanged.
- If a locked comment needs revision, add a new note outside the lock or ask the human reviewer to unlock it.


# State Machine Standards

## When to Use State Machines

- Device lifecycle management (boot -> configure -> run -> sleep -> wake)
- Communication protocol handling (connect -> auth -> subscribe -> active)
- Sensor sampling workflows (idle -> trigger -> wait -> read -> process)
- User interaction flows (idle -> input -> validate -> confirm -> execute)
- Any process with more than 3 distinct states

## Design Rules

- Define ALL states as an enum — no implicit states
- Define ALL valid transitions — reject invalid transitions explicitly
- Every state must have an entry action, a do action (optional), and an exit action
- Log every state transition with previous and new state
- Handle the "impossible" default case — don't leave switch statements without it

## Implementation Pattern

```c
typedef enum {
    STATE_INIT,
    STATE_CONNECTING,
    STATE_AUTHENTICATING,
    STATE_ACTIVE,
    STATE_ERROR,
    STATE_RECOVERY,
    STATE_COUNT  // Always last — used for validation
} device_state_t;

typedef enum {
    EVENT_BOOT,
    EVENT_CONNECTED,
    EVENT_AUTH_SUCCESS,
    EVENT_AUTH_FAILED,
    EVENT_DISCONNECT,
    EVENT_TIMEOUT,
    EVENT_RECOVERY_COMPLETE,
    EVENT_COUNT
} device_event_t;
```

## Transition Table Pattern (preferred over nested switch)

```c
// Explicit transition table — all valid transitions visible at a glance
static const state_transition_t transitions[] = {
    { STATE_INIT,           EVENT_BOOT,              STATE_CONNECTING,      action_start_connect },
    { STATE_CONNECTING,     EVENT_CONNECTED,         STATE_AUTHENTICATING,  action_start_auth    },
    { STATE_CONNECTING,     EVENT_TIMEOUT,           STATE_ERROR,           action_log_timeout   },
    { STATE_AUTHENTICATING, EVENT_AUTH_SUCCESS,       STATE_ACTIVE,          action_start_active  },
    { STATE_AUTHENTICATING, EVENT_AUTH_FAILED,        STATE_ERROR,           action_log_auth_fail },
    { STATE_ACTIVE,         EVENT_DISCONNECT,        STATE_RECOVERY,        action_start_recovery},
    { STATE_ERROR,          EVENT_RECOVERY_COMPLETE,  STATE_CONNECTING,      action_retry_connect },
};
```

## Rules

- Every state must be reachable from the initial state
- Every error state must have a recovery path (even if it's "reboot")
- Timeout every waiting state — no state should block indefinitely
- State machine must be testable — test every transition and every invalid transition
- Keep state data (context) separate from state logic
- One state machine per concern — don't multiplex unrelated logic into one machine
- Document the state diagram in comments or a separate doc file

