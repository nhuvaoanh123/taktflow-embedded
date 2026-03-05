#!/usr/bin/env python3
"""
Extract the combined TMS570LC4357 PDF (datasheet + LaunchPad QSG + TRM)
into per-section markdown files.

Source: hardware/datasheets/datasheet-1518516-evaluation-board-launchxl2-570lc43-hercules-hercules.pdf
Output: hardware/datasheets/combined-ds-sections/
"""
import PyPDF2
import os
import re
import time

PDF_PATH = os.path.join(os.path.dirname(__file__), '..', 'hardware', 'datasheets',
                        'datasheet-1518516-evaluation-board-launchxl2-570lc43-hercules-hercules.pdf')
OUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'hardware', 'datasheets', 'combined-ds-sections')

# (filename, title, start_page, end_page) - pages 1-indexed
SECTIONS = [
    # === DATASHEET (pages 1-227) ===
    ("ds-01-device-overview", "Device Overview", 1, 8),
    ("ds-02-revision-history", "Revision History", 7, 8),
    ("ds-03-device-comparison", "Device Comparison", 8, 8),
    ("ds-04-terminal-config-ball-map", "Terminal Configuration - Ball Map", 9, 10),
    ("ds-05-terminal-functions-adc", "Terminal Functions - ADC", 11, 13),
    ("ds-06-terminal-functions-n2het", "Terminal Functions - N2HET", 14, 17),
    ("ds-07-terminal-functions-rtp-ecap-eqep-epwm", "Terminal Functions - RTP, eCAP, eQEP, ePWM", 18, 23),
    ("ds-08-terminal-functions-dmm-gio", "Terminal Functions - DMM, GIO", 24, 25),
    ("ds-09-terminal-functions-flexray-dcan", "Terminal Functions - FlexRay, DCAN", 26, 27),
    ("ds-10-terminal-functions-lin-sci-i2c", "Terminal Functions - LIN, SCI, I2C", 28, 32),
    ("ds-11-terminal-functions-mibspi", "Terminal Functions - MibSPI", 31, 35),
    ("ds-12-terminal-functions-ethernet", "Terminal Functions - Ethernet", 33, 38),
    ("ds-13-terminal-functions-emif", "Terminal Functions - EMIF", 36, 40),
    ("ds-14-terminal-functions-debug-system", "Terminal Functions - Debug, System, Clocks", 39, 46),
    ("ds-15-terminal-functions-power-supply", "Terminal Functions - Power Supply, Ground", 43, 46),
    ("ds-16-output-multiplexing", "Output Multiplexing Table", 47, 52),
    ("ds-17-input-multiplexing", "Input Multiplexing Table", 53, 54),
    ("ds-18-specifications", "Specifications (Electrical, Timing)", 55, 63),
    ("ds-19-power-domains", "System Info - Power Domains", 64, 68),
    ("ds-20-cortex-r5f-cpu", "Cortex-R5F CPU Information", 69, 75),
    ("ds-21-clocks", "Clock Sources, PLL, Clock Domains", 76, 88),
    ("ds-22-memory-map", "Device Memory Map", 90, 100),
    ("ds-23-flash-memory", "Flash Memory", 101, 103),
    ("ds-24-sram", "On-Chip SRAM, PBIST", 104, 108),
    ("ds-25-emif", "External Memory Interface (EMIF)", 109, 116),
    ("ds-26-vim-interrupts", "VIM - Interrupt Request Assignments", 117, 120),
    ("ds-27-ecc-epc", "ECC Error Event Monitoring", 121, 122),
    ("ds-28-dma", "DMA Controller", 123, 126),
    ("ds-29-rti", "Real-Time Interrupt Module", 127, 128),
    ("ds-30-esm", "Error Signaling Module - Channel Assignments", 129, 137),
    ("ds-31-watchdog", "Digital Windowed Watchdog", 138, 138),
    ("ds-32-debug-jtag", "Debug Subsystem, JTAG", 139, 155),
    ("ds-33-epwm", "Enhanced PWM Modules", 156, 160),
    ("ds-34-ecap", "Enhanced Capture Modules", 161, 163),
    ("ds-35-eqep", "Enhanced Quadrature Encoder", 164, 165),
    ("ds-36-adc", "ADC Module", 166, 178),
    ("ds-37-gio", "General-Purpose I/O (GIO)", 179, 179),
    ("ds-38-n2het", "N2HET High-End Timer", 180, 184),
    ("ds-39-flexray", "FlexRay Interface", 185, 186),
    ("ds-40-dcan", "DCAN Controller Area Network", 187, 187),
    ("ds-41-lin", "LIN Interface", 188, 188),
    ("ds-42-sci", "SCI Serial Communication Interface", 189, 189),
    ("ds-43-i2c", "Inter-Integrated Circuit (I2C)", 190, 192),
    ("ds-44-mibspi", "Multi-Buffered SPI (MibSPI)", 193, 206),
    ("ds-45-ethernet", "Ethernet MAC", 207, 210),
    ("ds-46-applications", "Applications, Implementation, Layout", 211, 211),
    ("ds-47-documentation-support", "Device and Documentation Support", 212, 222),
    ("ds-48-mechanical", "Mechanical Data, Packaging", 223, 227),

    # === LAUNCHPAD QSG (pages 228-231) ===
    ("qsg-01-launchpad-quickstart", "LAUNCHXL2-570LC43 Quick Start Guide", 228, 231),
]


def sanitize(text):
    replacements = {
        '\u2002': ' ', '\u2003': ' ', '\u2011': '-', '\u2013': '-',
        '\u2014': '--', '\u2018': "'", '\u2019': "'", '\u201c': '"',
        '\u201d': '"', '\u2022': '*', '\u00a0': ' ', '\u2026': '...',
        '\ufb01': 'fi', '\ufb02': 'fl',
    }
    for old, new in replacements.items():
        text = text.replace(old, new)
    text = re.sub(r'[\ue000-\uf8ff]', '', text)
    text = re.sub(r' {3,}', '  ', text)
    text = re.sub(r'[ \t]+\n', '\n', text)
    text = re.sub(r'\n{4,}', '\n\n\n', text)
    return text.strip()


def extract_pages(reader, start_page, end_page):
    parts = []
    for i in range(start_page - 1, min(end_page, len(reader.pages))):
        try:
            text = reader.pages[i].extract_text()
            if text:
                parts.append(f"\n<!-- Page {i+1} -->\n{sanitize(text)}")
        except Exception as e:
            parts.append(f"\n<!-- Page {i+1}: error: {e} -->\n")
    return '\n'.join(parts)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    print(f"Reading: {PDF_PATH}")
    reader = PyPDF2.PdfReader(PDF_PATH)
    print(f"Total pages: {len(reader.pages)}")

    for filename, title, start, end in SECTIONS:
        out_path = os.path.join(OUT_DIR, f"{filename}.md")
        print(f"  [{start}-{end}] {title} ...", end='', flush=True)

        content = extract_pages(reader, start, end)
        num_pages = end - start + 1

        with open(out_path, 'w', encoding='utf-8') as f:
            f.write(f"# {title}\n\n")
            f.write(f"> **Source:** TMS570LC4357 Combined Datasheet\n")
            f.write(f"> **PDF:** datasheet-1518516-evaluation-board-launchxl2-570lc43-hercules-hercules.pdf\n")
            f.write(f"> **Pages:** {start}-{end} ({num_pages} pages)\n")
            f.write(f"> **Extracted:** {time.strftime('%Y-%m-%d')}\n\n---\n\n")
            f.write(content)

        size = os.path.getsize(out_path)
        print(f" {size:,} bytes")

    print(f"\nDone! {len(SECTIONS)} files in {OUT_DIR}")
    total = sum(os.path.getsize(os.path.join(OUT_DIR, f"{fn}.md")) for fn, _, _, _ in SECTIONS)
    print(f"Total size: {total:,} bytes ({total/1024:.0f} KB)")


if __name__ == '__main__':
    main()
