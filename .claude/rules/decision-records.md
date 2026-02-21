# Decision Record Requirements

## Rule

Every architectural, design, process, or tool decision MUST include:

1. **Tier**: classification (see Decision Tiers below)
2. **Rationale**: WHY this option was chosen
3. **Effort**: cost ($) and time (hours/days) for the chosen option
4. **Alternative A**: description, effort ($ + time), pros, cons
5. **Alternative B**: description, effort ($ + time), pros, cons
6. **Why chosen wins**: explicit comparison against alternatives on effort and suitability

## Decision Tiers

Every decision is classified into one of 4 tiers based on impact and reversibility:

| Tier | Name | Reversibility | Impact | Examples |
|------|------|---------------|--------|----------|
| **T1** | **Architecture** | Locked (months+ to reverse) | Defines system shape | MCU selection, bus protocol, safety architecture, ECU count |
| **T2** | **Design** | Hard (weeks to reverse) | Affects multiple components | BSW structure, test framework, cloud stack, simulation strategy |
| **T3** | **Implementation** | Moderate (days to reverse) | Affects one component | Library choice, driver approach, sensor model, API style |
| **T4** | **Process** | Easy (hours to reverse) | Affects workflow only | Folder structure, naming convention, doc format, branching strategy |

### Scoring Dimensions

Each decision also gets a 1-3 score on four dimensions:

| Dimension | 1 (Low) | 2 (Medium) | 3 (High) |
|-----------|---------|------------|----------|
| **Cost** | < $50 | $50–$500 | > $500 |
| **Time** | < 1 week | 1–4 weeks | > 4 weeks |
| **Safety** | QM / no safety impact | ASIL A–C | ASIL D |
| **Resume** | Generic skill | Industry-relevant | Top automotive keyword |

**Impact score** = sum of the 4 dimension scores (range: 4–12).

### Tier Assignment Rules

- **T1**: Impact score >= 9 OR safety dimension = 3 OR cost dimension = 3
- **T2**: Impact score 7–8 OR affects 3+ components
- **T3**: Impact score 5–6 OR affects exactly 1 component
- **T4**: Impact score <= 4 OR purely process/workflow

## Where to Record

- **Full ADR**: `docs/aspice/plans/MAN.3-project-management/decision-log.md`
- **Summary**: `docs/aspice/plans/MAN.3-project-management/decision-summary.md`
- **Inline tag**: Add `<!-- DECISION: ADR-NNN — short title -->` at the decision point in any doc
- **ADR ID**: Sequential `ADR-NNN` starting from ADR-001

## Format Template

```markdown
## ADR-NNN: {Decision Title}

- **Date**: YYYY-MM-DD
- **Tier**: T{1-4} — {Architecture|Design|Implementation|Process}
- **Scores**: Cost {1-3} | Time {1-3} | Safety {1-3} | Resume {1-3} = **{total}/12**
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
