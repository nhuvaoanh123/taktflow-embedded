"""Data model — intermediate representation consumed by all generators."""
from __future__ import annotations

from dataclasses import dataclass, field


# ── Atomic entries ──────────────────────────────────────────────────

@dataclass
class RteSignalEntry:
    """One RTE signal (id >= 16)."""
    id: int
    macro_name: str          # e.g. ABS_SIG_WHEEL_SPEED_FL
    c_type: str              # uint8, uint16, sint16, uint32
    initial_value: int


@dataclass
class ComSignalEntry:
    """One Com-layer signal mapped to a PDU."""
    signal_id: int           # sequential Com signal ID (0-based)
    bit_pos: int             # absolute bit position in PDU byte array
    bit_size: int            # signal width in bits
    c_type: str              # COM_UINT8 | COM_UINT16 | COM_SINT16
    pdu_macro: str           # parent PDU macro name
    shadow_name: str         # C variable name for shadow buffer
    is_tx: bool
    rte_signal_macro: str    # corresponding RTE signal macro (for bridge)


@dataclass
class ComTxPduEntry:
    """One Com TX PDU (one CAN message to transmit)."""
    pdu_macro: str           # e.g. ABS_COM_TX_BRAKE_CMD
    pdu_value: int           # PDU ID value (0-based per direction)
    can_id: int              # CAN arbitration ID from DBC
    dlc: int                 # data length code (bytes)
    cycle_ms: int            # TX period


@dataclass
class ComRxPduEntry:
    """One Com RX PDU (one CAN message to receive)."""
    pdu_macro: str           # e.g. ABS_COM_RX_WHEEL_SPEED
    pdu_value: int           # PDU ID value (0-based per direction)
    can_id: int              # CAN arbitration ID from DBC
    dlc: int                 # data length code (bytes)
    timeout_ms: int          # RX timeout


@dataclass
class RunnableEntry:
    """One runnable (SWC periodic function)."""
    func: str                # C function name
    period_ms: int
    priority: int
    init_func: str | None    # init function (called once at startup)


@dataclass
class FaultScenarioEntry:
    """One fault injection scenario."""
    name: str                # e.g. wheel_speed_loss
    scenario_type: str       # can_timeout | can_inject | container_stop
    target_message: str      # DBC message name


# ── Aggregate model ─────────────────────────────────────────────────

@dataclass
class EcuModel:
    """Complete generator input for one customer ECU."""

    # Naming variants
    name: str                # 'abs'
    name_upper: str          # 'ABS'
    name_pascal: str         # 'Abs'

    can_baudrate: int

    # Customer SWC files (copied into output)
    swc_sources: list[str] = field(default_factory=list)
    swc_headers: list[str] = field(default_factory=list)

    # Signal / PDU tables
    rte_signals: list[RteSignalEntry] = field(default_factory=list)
    com_signals: list[ComSignalEntry] = field(default_factory=list)
    tx_pdus: list[ComTxPduEntry] = field(default_factory=list)
    rx_pdus: list[ComRxPduEntry] = field(default_factory=list)

    # Runnables
    runnables: list[RunnableEntry] = field(default_factory=list)

    # Fault injection
    fault_scenarios: list[FaultScenarioEntry] = field(default_factory=list)
