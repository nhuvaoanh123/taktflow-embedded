"""Generate scenarios_<ecu>.py — fault injection scenario functions."""
from __future__ import annotations

from tools.onboard.generator._render import render_template
from tools.onboard.data_model import EcuModel


def generate(model: EcuModel) -> list[tuple[str, str]]:
    if not model.fault_scenarios:
        return []

    # Build CAN ID lookup for fault scenario targets
    # (tx_pdus + rx_pdus both have can_id; fault targets are DBC message names
    #  which map to PDU macros containing the message name)
    dbc_can_ids: dict[str, int] = {}
    for pdu in model.tx_pdus:
        # Extract message name from macro: ABS_COM_TX_ABS_BRAKECMD → ABS_BrakeCmd
        # We store the original DBC message name from fault_scenarios
        pass
    for pdu in model.rx_pdus:
        pass

    # Build lookup from all PDUs — match by CAN ID
    all_pdus = {**{p.pdu_macro: p.can_id for p in model.tx_pdus},
                **{p.pdu_macro: p.can_id for p in model.rx_pdus}}

    # Match fault scenario target_message to PDU by checking if the
    # uppercased target message name appears in any PDU macro
    for fs in model.fault_scenarios:
        target_upper = fs.target_message.upper()
        for macro, can_id in all_pdus.items():
            if target_upper in macro:
                dbc_can_ids[fs.target_message] = can_id
                break
        if fs.target_message not in dbc_can_ids:
            # Fallback: use 0x000 if not found
            dbc_can_ids[fs.target_message] = 0

    content = render_template(
        "fault_scenarios.j2",
        ecu=model,
        dbc_can_ids=dbc_can_ids,
    )
    return [(f"fault_inject/scenarios_{model.name}.py", content)]
