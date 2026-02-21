# Decision Record Requirements

## Rule

Every architectural, design, process, or tool decision MUST include:

1. **Rationale**: WHY this option was chosen
2. **Effort**: cost ($) and time (hours/days) for the chosen option
3. **Alternative A**: description, effort ($ + time), pros, cons
4. **Alternative B**: description, effort ($ + time), pros, cons
5. **Why chosen wins**: explicit comparison against alternatives on effort and suitability

## Where to Record

- **Primary**: `docs/aspice/plans/MAN.3-project-management/decision-log.md`
- **Inline tag**: Add `<!-- DECISION: ADR-NNN — short title -->` at the decision point in any doc
- **ADR ID**: Sequential `ADR-NNN` starting from ADR-001

## Format Template

```markdown
## ADR-NNN: {Decision Title}

- **Date**: YYYY-MM-DD
- **Decision**: {What was chosen}
- **Rationale**: {Why}
- **Effort**: {$ cost + time estimate}

### Alternative A: {Name}
- **Effort**: {$ cost + time estimate}
- **Pros**: {benefits}
- **Cons**: {drawbacks}

### Alternative B: {Name}
- **Effort**: {$ cost + time estimate}
- **Pros**: {benefits}
- **Cons**: {drawbacks}

### Why chosen wins
{Explicit comparison on effort, suitability, and project constraints}
```

## Daily Audit

Run `scripts/decision-audit.sh` daily to find non-compliant decisions.

The script checks:
1. All ADRs in decision-log.md have required fields
2. All `<!-- DECISION: -->` tags reference a valid ADR
3. Heuristic grep for untagged decisions in docs/

## What Counts as a Decision

- Architecture choices (MCU, protocol, framework, topology)
- Tool choices (IDE, test framework, CI, cloud provider)
- Process choices (branching strategy, review process, coverage targets)
- Design patterns (layered architecture, state machine style, error handling approach)

## What Does NOT Need an ADR

- Standard/regulation mandates (ISO 26262, MISRA — these are requirements, not choices)
- Trivial formatting/naming choices
- Temporary debugging decisions
- Decisions already documented with full justification in external standards
