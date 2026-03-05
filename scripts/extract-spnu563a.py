#!/usr/bin/env python3
"""
Extract spnu563a.pdf (TMS570LC43x TRM) into per-chapter markdown files.
One file per chapter heading.
"""
import PyPDF2
import os
import sys

PDF_PATH = os.path.join(os.path.dirname(__file__), '..', 'hardware', 'datasheets', 'spnu563a.pdf')
OUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'hardware', 'datasheets', 'spnu563a-sections')

# Chapter definitions: (filename, title, start_page, end_page)
# Pages are 1-indexed (will subtract 1 for PyPDF2)
CHAPTERS = [
    ("00-table-of-contents", "Table of Contents", 2, 103),
    ("01-introduction", "Introduction", 106, 111),
    ("02-architecture", "Architecture", 112, 251),
    ("02a-architecture-memory-map", "Architecture - Memory Map", 120, 141),
    ("02b-architecture-clocks", "Architecture - Clocks", 142, 150),
    ("02c-architecture-system-control-regs", "Architecture - System Control Registers (SYS)", 151, 204),
    ("02d-architecture-sys2-regs", "Architecture - Secondary System Control Registers (SYS2)", 205, 216),
    ("02e-architecture-pcr-regs", "Architecture - Peripheral Central Resource (PCR) Registers", 217, 251),
    ("03-scr-control-module", "SCR Control Module (SCM)", 252, 264),
    ("04-interconnect", "Interconnect", 265, 278),
    ("05-power-management", "Power Management Module (PMM)", 279, 300),
    ("06-iomm-pin-muxing", "I/O Multiplexing and Control Module (IOMM)", 301, 337),
    ("07-flash-controller", "F021 Level 2 Flash Module Controller (L2FMC)", 338, 386),
    ("08-l2ram", "Level 2 RAM (L2RAMW) Module", 387, 404),
    ("09-pbist", "Programmable Built-In Self-Test (PBIST)", 405, 427),
    ("10-stc", "Self-Test Controller (STC) Module", 428, 459),
    ("11-nmpu", "System Memory Protection Unit (NMPU)", 460, 482),
    ("12-epc", "Error Profiling Controller (EPC)", 483, 496),
    ("13-ccm-r5f", "CPU Compare Module for Cortex-R5F (CCM-R5F)", 497, 516),
    ("14-oscillator-pll", "Oscillator and PLL", 517, 541),
    ("15-dcc", "Dual-Clock Comparator (DCC) Module", 542, 557),
    ("16-esm", "Error Signaling Module (ESM)", 558, 582),
    ("17-rti", "Real-Time Interrupt (RTI) Module", 583, 624),
    ("18-crc", "Cyclic Redundancy Check (CRC) Controller", 625, 661),
    ("19-vim", "Vectored Interrupt Manager (VIM) Module", 662, 695),
    ("20-dma", "Direct Memory Access Controller (DMA) Module", 696, 792),
    ("21-emif", "External Memory Interface (EMIF)", 793, 847),
    ("22-adc", "Analog To Digital Converter (ADC) Module", 848, 952),
    ("23-n2het", "High-End Timer (N2HET) Module", 953, 1130),
    ("24-htu", "High-End Timer Transfer Unit (HTU) Module", 1131, 1182),
    ("25-gio", "General-Purpose Input/Output (GIO) Module", 1183, 1209),
    ("26-flexray", "FlexRay Module", 1210, 1416),
    ("27-dcan", "Controller Area Network (DCAN) Module", 1417, 1496),
    ("28-mibspi", "Multi-Buffered Serial Peripheral Interface (MibSPI)", 1497, 1620),
    ("29-sci-lin", "Serial Communication Interface (SCI) / LIN Module", 1621, 1716),
    ("30-sci", "Serial Communication Interface (SCI) Standalone Module", 1717, 1764),
    ("31-i2c", "Inter-Integrated Circuit (I2C) Module", 1765, 1802),
    ("32-emac-mdio", "EMAC/MDIO Module", 1803, 1926),
    ("33-ecap", "Enhanced Capture (eCAP) Module", 1927, 1956),
    ("34-eqep", "Enhanced Quadrature Encoder Pulse (eQEP) Module", 1957, 1994),
    ("35-epwm", "Enhanced Pulse Width Modulator (ePWM) Module", 1995, 2107),
    ("36-dmm", "Data Modification Module (DMM)", 2108, 2153),
    ("37-rtp", "RAM Trace Port (RTP)", 2154, 2186),
    ("38-efuse", "eFuse Controller", 2187, 2195),
]


def sanitize(text):
    """Clean up Unicode characters from PDF extraction."""
    replacements = {
        '\u2002': ' ',   # en space
        '\u2003': ' ',   # em space
        '\u2011': '-',   # non-breaking hyphen
        '\u2013': '-',   # en dash
        '\u2014': '--',  # em dash
        '\u2018': "'",   # left single quote
        '\u2019': "'",   # right single quote
        '\u201c': '"',   # left double quote
        '\u201d': '"',   # right double quote
        '\u2022': '*',   # bullet
        '\u00a0': ' ',   # non-breaking space
        '\u2026': '...', # ellipsis
        '\ufb01': 'fi',  # fi ligature
        '\ufb02': 'fl',  # fl ligature
    }
    for old, new in replacements.items():
        text = text.replace(old, new)
    return text


def extract_pages(reader, start_page, end_page):
    """Extract text from a range of pages (1-indexed)."""
    text_parts = []
    for i in range(start_page - 1, min(end_page, len(reader.pages))):
        try:
            page_text = reader.pages[i].extract_text()
            if page_text:
                text_parts.append(f"\n<!-- Page {i+1} -->\n")
                text_parts.append(sanitize(page_text))
        except Exception as e:
            text_parts.append(f"\n<!-- Page {i+1}: extraction error: {e} -->\n")
    return '\n'.join(text_parts)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    print(f"Reading PDF: {PDF_PATH}")
    reader = PyPDF2.PdfReader(PDF_PATH)
    print(f"Total pages: {len(reader.pages)}")

    for filename, title, start, end in CHAPTERS:
        out_path = os.path.join(OUT_DIR, f"{filename}.md")
        print(f"  Extracting [{start}-{end}] {title} ...", end='', flush=True)

        content = extract_pages(reader, start, end)

        with open(out_path, 'w', encoding='utf-8') as f:
            f.write(f"# {title}\n\n")
            f.write(f"> **Source:** spnu563a.pdf (TMS570LC43x Technical Reference Manual)\n")
            f.write(f"> **Pages:** {start}-{end}\n\n")
            f.write("---\n\n")
            f.write(content)

        size = os.path.getsize(out_path)
        print(f" {size:,} bytes")

    print(f"\nDone! Files written to: {OUT_DIR}")

    # Summary
    total_size = 0
    for filename, title, start, end in CHAPTERS:
        path = os.path.join(OUT_DIR, f"{filename}.md")
        size = os.path.getsize(path)
        total_size += size
    print(f"Total: {len(CHAPTERS)} files, {total_size:,} bytes ({total_size/1024/1024:.1f} MB)")


if __name__ == '__main__':
    main()
