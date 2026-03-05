"""Generate Swc_<Ecu>Com.c and Swc_<Ecu>Com.h — Com↔RTE bridge."""
from __future__ import annotations

from tools.onboard.generator._render import render_template
from tools.onboard.data_model import EcuModel


def generate(model: EcuModel) -> list[tuple[str, str]]:
    tx_sigs = [s for s in model.com_signals if s.is_tx]
    rx_sigs = [s for s in model.com_signals if not s.is_tx]

    c_content = render_template(
        "swc_com_c.j2",
        ecu=model,
        tx_com_signals=tx_sigs,
        rx_com_signals=rx_sigs,
    )
    h_content = render_template("swc_com_h.j2", ecu=model)

    return [
        (f"{model.name}/src/Swc_{model.name_pascal}Com.c", c_content),
        (f"{model.name}/src/Swc_{model.name_pascal}Com.h", h_content),
    ]
