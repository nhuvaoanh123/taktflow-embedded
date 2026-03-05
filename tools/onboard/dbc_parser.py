"""Parse a DBC file using cantools, return a thin wrapper for the resolver."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import cantools


@dataclass(frozen=True)
class DbcSignalInfo:
    """Resolved info for one DBC signal."""
    name: str
    start_bit: int
    length: int            # in bits
    is_signed: bool
    byte_order: str        # 'little_endian' | 'big_endian'


@dataclass(frozen=True)
class DbcMessageInfo:
    """Resolved info for one DBC message."""
    name: str
    frame_id: int          # CAN arbitration ID
    dlc: int               # data length code (bytes)
    signals: dict[str, DbcSignalInfo]


class DbcDatabase:
    """Thin wrapper around cantools.database.Database."""

    def __init__(self, db: cantools.database.Database) -> None:
        self._db = db
        self._messages: dict[str, DbcMessageInfo] = {}
        for msg in db.messages:
            sigs = {}
            for sig in msg.signals:
                sigs[sig.name] = DbcSignalInfo(
                    name=sig.name,
                    start_bit=sig.start,
                    length=sig.length,
                    is_signed=sig.is_signed,
                    byte_order=sig.byte_order,
                )
            self._messages[msg.name] = DbcMessageInfo(
                name=msg.name,
                frame_id=msg.frame_id,
                dlc=msg.length,
                signals=sigs,
            )

    @property
    def messages(self) -> dict[str, DbcMessageInfo]:
        return self._messages

    def get_message(self, name: str) -> DbcMessageInfo | None:
        return self._messages.get(name)


def parse_dbc(dbc_path: str | Path) -> DbcDatabase:
    """Parse a .dbc file and return a DbcDatabase."""
    dbc_path = Path(dbc_path)
    if not dbc_path.exists():
        raise FileNotFoundError(f"DBC file not found: {dbc_path}")
    db = cantools.database.load_file(str(dbc_path))
    return DbcDatabase(db)
