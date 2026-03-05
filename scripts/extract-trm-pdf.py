#!/usr/bin/env python3
"""
extract-trm-pdf.py
==================
Extracts key sections from the TMS570LC4357 combined PDF (datasheet + LaunchPad QSG + TRM)
into individual Markdown files for easy reference.

Source PDF: hardware/datasheets/datasheet-1518516-evaluation-board-launchxl2-570lc43-hercules-hercules.pdf
Output:     hardware/datasheets/trm-sections/*.md

The combined PDF has 2359 pages:
  - Datasheet:     pages 1-227
  - LaunchPad QSG: pages 228-231
  - TRM:           pages 232-2359

Usage:
    python scripts/extract-trm-pdf.py
"""

import os
import sys
import re
import time
from pathlib import Path

try:
    from PyPDF2 import PdfReader
except ImportError:
    print("ERROR: PyPDF2 is required. Install with: pip install PyPDF2")
    sys.exit(1)


# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent  # taktflow-embedded/

PDF_PATH = PROJECT_ROOT / "hardware" / "datasheets" /     "datasheet-1518516-evaluation-board-launchxl2-570lc43-hercules-hercules.pdf"

OUTPUT_DIR = PROJECT_ROOT / "hardware" / "datasheets" / "trm-sections"
