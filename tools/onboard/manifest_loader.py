"""Load and validate a customer manifest YAML against the JSON schema."""
from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import jsonschema
import yaml

_SCHEMA_PATH = Path(__file__).parent / "schema" / "manifest.schema.json"


def _load_schema() -> dict[str, Any]:
    with open(_SCHEMA_PATH, encoding="utf-8") as f:
        return json.load(f)


def load_manifest(manifest_path: str | Path) -> dict[str, Any]:
    """Load a manifest YAML and validate it against the schema.

    Returns the validated manifest dict.
    Raises ``ManifestError`` on validation or parse failure.
    """
    manifest_path = Path(manifest_path)
    if not manifest_path.exists():
        raise ManifestError(f"Manifest file not found: {manifest_path}")

    with open(manifest_path, encoding="utf-8") as f:
        try:
            data = yaml.safe_load(f)
        except yaml.YAMLError as exc:
            raise ManifestError(f"YAML parse error: {exc}") from exc

    if not isinstance(data, dict):
        raise ManifestError("Manifest root must be a YAML mapping")

    schema = _load_schema()
    try:
        jsonschema.validate(instance=data, schema=schema)
    except jsonschema.ValidationError as exc:
        raise ManifestError(f"Schema validation failed: {exc.message}") from exc

    # Apply defaults that jsonschema doesn't auto-fill
    data.setdefault("fault_scenarios", [])
    data["ecu"].setdefault("can_baudrate", 500000)
    data["swc_files"].setdefault("headers", [])
    for sig in data["rte_signals"]:
        sig.setdefault("initial_value", 0)

    return data


class ManifestError(Exception):
    """Raised when manifest loading or validation fails."""
