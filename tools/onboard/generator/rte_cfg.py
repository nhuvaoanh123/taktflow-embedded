"""Generate Rte_Cfg_<Ecu>.c — BSW signals 0-15, app signals, runnable table."""
from __future__ import annotations

from tools.onboard.generator._render import render_template
from tools.onboard.data_model import EcuModel


def generate(model: EcuModel) -> list[tuple[str, str]]:
    content = render_template("rte_cfg_c.j2", ecu=model)
    path = f"{model.name}/cfg/Rte_Cfg_{model.name_pascal}.c"
    return [(path, content)]
