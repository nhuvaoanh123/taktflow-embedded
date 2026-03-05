"""Generate docker-compose.customer.yml."""
from __future__ import annotations

from tools.onboard.generator._render import render_template
from tools.onboard.data_model import EcuModel


def generate(model: EcuModel) -> list[tuple[str, str]]:
    content = render_template("compose.j2", ecu=model)
    return [("docker-compose.customer.yml", content)]
