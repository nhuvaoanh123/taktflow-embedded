"""Generate <ecu>_hw_posix.c — POSIX hardware stubs (all no-ops)."""
from __future__ import annotations

from tools.onboard.generator._render import render_template
from tools.onboard.data_model import EcuModel


def generate(model: EcuModel) -> list[tuple[str, str]]:
    content = render_template("hw_posix_c.j2", ecu=model)
    path = f"{model.name}/cfg/{model.name}_hw_posix.c"
    return [(path, content)]
