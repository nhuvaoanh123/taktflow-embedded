---
paths:
  - "hardware/**/*"
  - "docs/**/*"
---

# Procurement & BOM Rule

## Single Source of Truth

`hardware/bom.md` is the ONLY file that tracks procurement/delivery status.

### Rules
- When user reports an item arrived: update the Status column in `hardware/bom.md`
- NEVER write delivery status (Delivered, Arriving, ON HAND, Not ordered) into any other file
- Other files may reference BOM item numbers (e.g. "BOM #55") but must NOT include delivery dates or status
- If you see stale procurement status in a non-BOM file, replace it with a BOM reference
