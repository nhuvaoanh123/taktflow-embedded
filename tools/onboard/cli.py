"""CLI entry point: orchestrates manifest → DBC parse → resolve → generate."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Sequence

from tools.onboard.manifest_loader import ManifestError, load_manifest
from tools.onboard.dbc_parser import parse_dbc
from tools.onboard.resolver import resolve
from tools.onboard.generator import cfg_h, com_cfg, rte_cfg, swc_com, main_c, hw_posix
from tools.onboard.generator import makefile, dockerfile, compose, fault_scenarios


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="tools.onboard",
        description="SWC Onboarding Layer — generate SIL wiring from manifest + DBC",
    )
    sub = parser.add_subparsers(dest="command")

    gen = sub.add_parser("generate", help="Generate all wiring artifacts")
    gen.add_argument("--manifest", required=True, help="Path to manifest YAML")
    gen.add_argument("--output", required=True, help="Output directory")
    gen.add_argument("--verify", action="store_true", help="Compile + boot check after generation")
    gen.add_argument("--dry-run", action="store_true", help="Print files without writing")
    gen.add_argument("--force", action="store_true", help="Overwrite existing output")

    return parser


def _collect_files(model) -> list[tuple[str, str]]:
    """Run all generators, return list of (relative_path, content)."""
    files: list[tuple[str, str]] = []
    generators = [
        cfg_h, com_cfg, rte_cfg, swc_com, main_c, hw_posix,
        makefile, dockerfile, compose, fault_scenarios,
    ]
    for gen in generators:
        files.extend(gen.generate(model))
    return files


def _write_files(
    files: list[tuple[str, str]],
    output_dir: Path,
    *,
    dry_run: bool = False,
    force: bool = False,
) -> None:
    """Write generated files to output_dir."""
    for rel_path, content in files:
        dest = output_dir / rel_path
        if dest.exists() and not force:
            print(f"  SKIP (exists): {rel_path}", file=sys.stderr)
            continue
        if dry_run:
            print(f"  DRY-RUN: {rel_path} ({len(content)} bytes)")
            continue
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(content, encoding="utf-8")
        print(f"  WROTE: {rel_path}")


def main(argv: Sequence[str] | None = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)

    if args.command is None:
        parser.print_help()
        return 1

    if args.command == "generate":
        return _cmd_generate(args)

    return 0


def _cmd_generate(args: argparse.Namespace) -> int:
    manifest_path = Path(args.manifest)
    output_dir = Path(args.output)

    # 1. Load + validate manifest
    print(f"Loading manifest: {manifest_path}")
    try:
        manifest = load_manifest(manifest_path)
    except ManifestError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    ecu_name = manifest["ecu"]["name"]
    print(f"ECU: {ecu_name}")

    # 2. Parse DBC
    dbc_path = manifest_path.parent / manifest["dbc_file"]
    print(f"Parsing DBC: {dbc_path}")
    try:
        dbc = parse_dbc(str(dbc_path))
    except Exception as exc:
        print(f"ERROR parsing DBC: {exc}", file=sys.stderr)
        return 1

    # 3. Resolve manifest + DBC → EcuModel
    print("Resolving manifest + DBC -> EcuModel...")
    try:
        model = resolve(manifest, dbc)
    except Exception as exc:
        print(f"ERROR resolving: {exc}", file=sys.stderr)
        return 1

    # 4. Generate all files
    print("Generating files...")
    files = _collect_files(model)

    # 5. Write output
    _write_files(files, output_dir, dry_run=args.dry_run, force=args.force)

    print(f"\nGenerated {len(files)} files for ECU '{ecu_name}'")

    # 6. Optional verify
    if args.verify and not args.dry_run:
        print("\nRunning verify...")
        from tools.onboard.verify import run_verify
        return run_verify(output_dir, ecu_name)

    return 0
