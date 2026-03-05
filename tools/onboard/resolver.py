"""Cross-reference manifest YAML + DBC → EcuModel for generators."""
from __future__ import annotations

from typing import Any

from tools.onboard.data_model import (
    ComRxPduEntry, ComSignalEntry, ComTxPduEntry,
    EcuModel, FaultScenarioEntry, RteSignalEntry, RunnableEntry,
)
from tools.onboard.dbc_parser import DbcDatabase

# Type mapping: manifest type → (C Com type macro, bit size, C shadow type)
_TYPE_MAP = {
    "UINT8":  ("COM_UINT8",  8,  "uint8"),
    "UINT16": ("COM_UINT16", 16, "uint16"),
    "SINT16": ("COM_SINT16", 16, "sint16"),
    "UINT32": ("COM_UINT32", 32, "uint32"),
    "BOOL":   ("COM_UINT8",  8,  "uint8"),
}


class ResolveError(Exception):
    """Raised when manifest ↔ DBC cross-reference fails."""


def _to_pascal(name: str) -> str:
    """'abs' → 'Abs', 'abs_ctrl' → 'AbsCtrl'."""
    return "".join(part.capitalize() for part in name.split("_"))


def resolve(manifest: dict[str, Any], dbc: DbcDatabase) -> EcuModel:
    """Build an EcuModel from a validated manifest + parsed DBC."""
    ecu_cfg = manifest["ecu"]
    ecu_name = ecu_cfg["name"]
    ecu_upper = ecu_name.upper()
    ecu_pascal = _to_pascal(ecu_name)

    # ── RTE signals ──────────────────────────────────────────────
    rte_signals: list[RteSignalEntry] = []
    rte_by_name: dict[str, RteSignalEntry] = {}
    for sig_def in manifest["rte_signals"]:
        entry = RteSignalEntry(
            id=sig_def["id"],
            macro_name=f"{ecu_upper}_SIG_{sig_def['name']}",
            c_type=_TYPE_MAP[sig_def["type"]][2],
            initial_value=sig_def.get("initial_value", 0),
        )
        rte_signals.append(entry)
        rte_by_name[sig_def["name"]] = entry

    sig_count_macro_value = max(s.id for s in rte_signals) + 1

    # ── TX messages → TX PDUs + Com signals ──────────────────────
    tx_pdus: list[ComTxPduEntry] = []
    com_signals: list[ComSignalEntry] = []
    com_sig_id = 0

    for pdu_idx, tx_msg in enumerate(manifest["tx_messages"]):
        dbc_msg = dbc.get_message(tx_msg["dbc_message"])
        if dbc_msg is None:
            raise ResolveError(
                f"TX message '{tx_msg['dbc_message']}' not found in DBC"
            )

        pdu_macro = f"{ecu_upper}_COM_TX_{tx_msg['dbc_message'].upper()}"
        tx_pdus.append(ComTxPduEntry(
            pdu_macro=pdu_macro,
            pdu_value=pdu_idx,
            can_id=dbc_msg.frame_id,
            dlc=dbc_msg.dlc,
            cycle_ms=tx_msg["cycle_ms"],
        ))

        for sig_map in tx_msg["signals"]:
            dbc_sig = dbc_msg.signals.get(sig_map["dbc_signal"])
            if dbc_sig is None:
                raise ResolveError(
                    f"TX signal '{sig_map['dbc_signal']}' not found in "
                    f"DBC message '{tx_msg['dbc_message']}'"
                )
            rte_sig = rte_by_name.get(sig_map["rte_signal"])
            if rte_sig is None:
                raise ResolveError(
                    f"RTE signal '{sig_map['rte_signal']}' not found in "
                    f"rte_signals section"
                )

            # Determine Com type from RTE signal type
            rte_type_upper = next(
                k for k, v in _TYPE_MAP.items() if v[2] == rte_sig.c_type
            )
            com_type_macro, _, c_shadow_type = _TYPE_MAP[rte_type_upper]

            shadow_name = f"sig_tx_{sig_map['dbc_signal'].lower()}"
            com_signals.append(ComSignalEntry(
                signal_id=com_sig_id,
                bit_pos=dbc_sig.start_bit,
                bit_size=dbc_sig.length,
                c_type=com_type_macro,
                pdu_macro=pdu_macro,
                shadow_name=shadow_name,
                is_tx=True,
                rte_signal_macro=rte_sig.macro_name,
            ))
            com_sig_id += 1

    # ── RX messages → RX PDUs + Com signals ──────────────────────
    rx_pdus: list[ComRxPduEntry] = []

    for pdu_idx, rx_msg in enumerate(manifest["rx_messages"]):
        dbc_msg = dbc.get_message(rx_msg["dbc_message"])
        if dbc_msg is None:
            raise ResolveError(
                f"RX message '{rx_msg['dbc_message']}' not found in DBC"
            )

        pdu_macro = f"{ecu_upper}_COM_RX_{rx_msg['dbc_message'].upper()}"
        rx_pdus.append(ComRxPduEntry(
            pdu_macro=pdu_macro,
            pdu_value=pdu_idx,
            can_id=dbc_msg.frame_id,
            dlc=dbc_msg.dlc,
            timeout_ms=rx_msg["timeout_ms"],
        ))

        for sig_map in rx_msg["signals"]:
            dbc_sig = dbc_msg.signals.get(sig_map["dbc_signal"])
            if dbc_sig is None:
                raise ResolveError(
                    f"RX signal '{sig_map['dbc_signal']}' not found in "
                    f"DBC message '{rx_msg['dbc_message']}'"
                )
            rte_sig = rte_by_name.get(sig_map["rte_signal"])
            if rte_sig is None:
                raise ResolveError(
                    f"RTE signal '{sig_map['rte_signal']}' not found in "
                    f"rte_signals section"
                )

            rte_type_upper = next(
                k for k, v in _TYPE_MAP.items() if v[2] == rte_sig.c_type
            )
            com_type_macro, _, c_shadow_type = _TYPE_MAP[rte_type_upper]

            shadow_name = f"sig_rx_{sig_map['dbc_signal'].lower()}"
            com_signals.append(ComSignalEntry(
                signal_id=com_sig_id,
                bit_pos=dbc_sig.start_bit,
                bit_size=dbc_sig.length,
                c_type=com_type_macro,
                pdu_macro=pdu_macro,
                shadow_name=shadow_name,
                is_tx=False,
                rte_signal_macro=rte_sig.macro_name,
            ))
            com_sig_id += 1

    # ── Runnables ────────────────────────────────────────────────
    runnables = [
        RunnableEntry(
            func=r["func"],
            period_ms=r["period_ms"],
            priority=r["priority"],
            init_func=r.get("init_func"),
        )
        for r in manifest["runnables"]
    ]

    # ── Fault scenarios ──────────────────────────────────────────
    fault_scenarios = [
        FaultScenarioEntry(
            name=f["name"],
            scenario_type=f["type"],
            target_message=f["target_message"],
        )
        for f in manifest.get("fault_scenarios", [])
    ]
    # Validate fault scenario targets exist in DBC
    for fs in fault_scenarios:
        if dbc.get_message(fs.target_message) is None:
            raise ResolveError(
                f"Fault scenario '{fs.name}' targets message "
                f"'{fs.target_message}' which is not in the DBC"
            )

    return EcuModel(
        name=ecu_name,
        name_upper=ecu_upper,
        name_pascal=ecu_pascal,
        can_baudrate=ecu_cfg.get("can_baudrate", 500000),
        swc_sources=manifest["swc_files"]["sources"],
        swc_headers=manifest["swc_files"].get("headers", []),
        rte_signals=rte_signals,
        com_signals=com_signals,
        tx_pdus=tx_pdus,
        rx_pdus=rx_pdus,
        runnables=runnables,
        fault_scenarios=fault_scenarios,
    )
