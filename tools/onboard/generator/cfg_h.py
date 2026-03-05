"""Generate <Ecu>_Cfg.h — signal IDs, PDU IDs, counts."""
from __future__ import annotations

from tools.onboard.generator._render import render_template
from tools.onboard.data_model import EcuModel


def generate(model: EcuModel) -> list[tuple[str, str]]:
    sig_count = max(s.id for s in model.rte_signals) + 1
    content = render_template("cfg_h.j2", ecu=model, sig_count=sig_count)
    path = f"{model.name}/cfg/{model.name_pascal}_Cfg.h"
    return [(path, content)]
