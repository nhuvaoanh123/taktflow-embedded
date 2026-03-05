"""Generate Dockerfile.customer — multi-stage build."""
from __future__ import annotations

from tools.onboard.generator._render import render_template
from tools.onboard.data_model import EcuModel


def generate(model: EcuModel) -> list[tuple[str, str]]:
    content = render_template("dockerfile.j2", ecu=model)
    return [("Dockerfile.customer", content)]
