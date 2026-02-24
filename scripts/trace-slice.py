#!/usr/bin/env python3
import argparse
import csv
import re
from pathlib import Path


def num_suffix(req_id: str) -> int:
    m = re.search(r"-([0-9]+)$", req_id)
    return int(m.group(1)) if m else -1


def in_range(req_id: str, from_id: str | None, to_id: str | None) -> bool:
    n = num_suffix(req_id)
    if from_id and n < num_suffix(from_id):
        return False
    if to_id and n > num_suffix(to_id):
        return False
    return True


def extract_stk_sys(text: str) -> list[tuple[str, str]]:
    rows: set[tuple[str, str]] = set()
    cur_sys = ""
    for line in text.splitlines():
        m = re.match(r"^###\s+(SYS-[0-9]+):", line)
        if m:
            cur_sys = m.group(1)
            continue
        if "- **Traces up**:" in line and cur_sys:
            for stk in re.findall(r"(STK-[0-9]+)", line):
                rows.add((stk, cur_sys))
    return sorted(rows, key=lambda x: (num_suffix(x[0]), num_suffix(x[1])))


def extract_sys_swr(text: str) -> list[tuple[str, str]]:
    rows: set[tuple[str, str]] = set()
    cur_sys = ""
    for line in text.splitlines():
        m = re.match(r"^###\s+(SYS-[0-9]+):", line)
        if m:
            cur_sys = m.group(1)
            continue
        if "- **Traces down**:" in line and cur_sys:
            for swr in re.findall(r"(SWR-[A-Z]+-[0-9]+)", line):
                rows.add((cur_sys, swr))
    return sorted(rows, key=lambda x: (num_suffix(x[0]), x[1]))


def write_csv(rows: list[tuple[str, str]], header: tuple[str, str], out_path: Path | None) -> None:
    if out_path:
        out_path.parent.mkdir(parents=True, exist_ok=True)
        f = out_path.open("w", newline="", encoding="utf-8")
    else:
        import sys

        f = sys.stdout
    with f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Scoped trace extraction to avoid large-context AI runs."
    )
    parser.add_argument("--layer", required=True, choices=["stk-sys", "sys-swr"])
    parser.add_argument("--from", dest="from_id")
    parser.add_argument("--to", dest="to_id")
    parser.add_argument("--out")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    sysreq = root / "docs" / "aspice" / "system" / "system-requirements.md"
    if not sysreq.exists():
        raise SystemExit(f"Missing file: {sysreq}")

    text = sysreq.read_text(encoding="utf-8")

    if args.layer == "stk-sys":
        rows = [r for r in extract_stk_sys(text) if in_range(r[0], args.from_id, args.to_id)]
        header = ("STK_ID", "SYS_ID")
    else:
        rows = [r for r in extract_sys_swr(text) if in_range(r[0], args.from_id, args.to_id)]
        header = ("SYS_ID", "SWR_ID")

    out_path = (root / args.out) if args.out else None
    write_csv(rows, header, out_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

