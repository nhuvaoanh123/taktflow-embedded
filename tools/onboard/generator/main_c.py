"""Generate <ecu>_main.c — BCM-pattern init + loop + CanIf routing."""
from __future__ import annotations

from tools.onboard.generator._render import render_template
from tools.onboard.data_model import EcuModel


def generate(model: EcuModel) -> list[tuple[str, str]]:
    content = render_template("main_c.j2", ecu=model)
    path = f"{model.name}/src/{model.name}_main.c"
    return [(path, content)]
