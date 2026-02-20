---
paths:
  - "**/*"
---

# Automotive SPICE (ASPICE) 4.0 Process Requirements

## Overview

ASPICE is the process assessment model used by automotive OEMs to evaluate supplier development maturity. Maintained by VDA QMC. Aligned with ISO/IEC 33xxx.

**Minimum OEM expectation**: ASPICE Level 2 for all assessed processes.
**Target for safety-critical (ASIL D)**: ASPICE Level 3.

## Capability Levels

| Level | Name | Description |
|-------|------|-------------|
| 0 | Incomplete | Process not implemented or fails its purpose |
| 1 | Performed | Process achieves its purpose; work products exist |
| **2** | **Managed** | **Process planned, monitored, controlled. Minimum OEM requirement.** |
| **3** | **Established** | **Standard process defined at org level, tailored per project. Target for ASIL D.** |
| 4 | Predictable | Quantitative process objectives; measurement-driven |
| 5 | Innovating | Continuous improvement; causal analysis |

## V-Model Mapping

```
LEFT SIDE (Specification)              RIGHT SIDE (Verification)
=========================              =========================
SYS.1 Requirements Elicitation
SYS.2 System Requirements      <--->  SYS.5 System Verification
SYS.3 System Architecture      <--->  SYS.4 System Integration & Test
SWE.1 SW Requirements          <--->  SWE.6 SW Verification
SWE.2 SW Architecture          <--->  SWE.5 SW Component Verification & Integration
SWE.3 SW Detailed Design       <--->  SWE.4 SW Unit Verification
       & Unit Construction
```

Every left-side activity has a corresponding right-side verification activity.

## Process Areas

### System Engineering (SYS)

| Process | Name | Key Output |
|---------|------|------------|
| SYS.1 | Requirements Elicitation | Stakeholder requirements specification |
| SYS.2 | System Requirements Analysis | System requirements specification (SRS), verification criteria |
| SYS.3 | System Architectural Design | System architecture, interface control document |
| SYS.4 | System Integration & Test | Integration test plan, test results |
| SYS.5 | System Verification | System verification test plan, test results |

### Software Engineering (SWE)

| Process | Name | Key Output |
|---------|------|------------|
| SWE.1 | SW Requirements Analysis | Software requirements specification, verification criteria |
| SWE.2 | SW Architectural Design | SW architecture (static + dynamic), interfaces, resource estimates |
| SWE.3 | SW Detailed Design & Unit Construction | Detailed design, source code |
| SWE.4 | SW Unit Verification | Unit test plan, test results, static analysis results, coverage |
| SWE.5 | SW Component Verification & Integration | Integration strategy, integration test results |
| SWE.6 | SW Verification | SW qualification test plan, test results, release notes |

### Hardware Engineering (NEW in ASPICE 4.0)

| Process | Name |
|---------|------|
| HWE.1 | Hardware Requirements Analysis |
| HWE.2 | Hardware Design |
| HWE.3 | Hardware Design Verification |
| HWE.4 | Hardware Requirements Verification |

### Support Processes (SUP)

| Process | Name | Purpose |
|---------|------|---------|
| SUP.1 | Quality Assurance | Independent QA of work products and processes |
| SUP.2 | Verification | Confirm work products meet requirements |
| SUP.4 | Joint Review | Common understanding with stakeholders |
| SUP.7 | Documentation | Develop and maintain process documentation |
| SUP.8 | Configuration Management | Version control, baselines, CM audits |
| SUP.9 | Problem Resolution | Track and resolve problems |
| SUP.10 | Change Request Management | Manage change requests |

### Management (MAN)

| Process | Name |
|---------|------|
| MAN.3 | Project Management |

### Acquisition (ACQ)

| Process | Name |
|---------|------|
| ACQ.4 | Supplier Monitoring |

## Work Products (Information Items in 4.0)

Each process produces defined work products. Key ones:

| Process | Work Products |
|---------|---------------|
| SYS.2 | System Requirements Specification, verification criteria |
| SYS.3 | System Architecture, HSI, integration test specification |
| SWE.1 | Software Requirements Specification, verification criteria |
| SWE.2 | Software Architecture Description, interface descriptions |
| SWE.3 | Detailed design, source code |
| SWE.4 | Unit test plan, test specs, test results, static analysis, coverage |
| SWE.5 | Integration strategy, integration test specs, test results |
| SWE.6 | SW verification test plan, test specs, test results, release notes |
| SUP.8 | CM plan, baselines, config status reports, CM audit results |
| MAN.3 | Project plan, schedule, WBS, risk assessment, progress reports |

**ASPICE 4.0 change**: "Work Products" renamed to "Information Items" — emphasizes content over format. Information can live in tools (Jira, DOORS, Git) rather than standalone documents.

## Configuration Management (SUP.8) Base Practices

| BP | Description |
|----|-------------|
| BP1 | Develop CM strategy (tools, naming, branching, access rights) |
| BP2 | Identify configuration items |
| BP3 | Establish branch management strategy |
| BP4 | Manage CI modifications |
| BP5 | Control modifications and releases |
| BP6 | Establish baselines (freeze development states) |
| BP7 | Report configuration status |
| BP8 | Verify CI completeness and correctness |
| BP9 | Manage storage (archive, backup, integrity) |

## ASPICE + ISO 26262 Relationship

- ASPICE assesses **process maturity** (how well-organized)
- ISO 26262 assesses **safety integrity** (how safe the product)
- Both are required by OEMs for safety-critical projects
- Mature processes (ASPICE L2/L3) reduce systematic faults
- ISO 26262 provides safety-specific analysis and validation
- They are complementary, not interchangeable
