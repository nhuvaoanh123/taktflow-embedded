"""Shared Jinja2 rendering helper for all generators."""
from __future__ import annotations

from datetime import date
from pathlib import Path

import jinja2

_TEMPLATE_DIR = Path(__file__).parent.parent / "templates"

# Com type macro → C declaration type
_C_TYPE_DECL_MAP = {
    "COM_UINT8":  "uint8",
    "COM_UINT16": "uint16",
    "COM_SINT16": "sint16",
    "COM_UINT32": "uint32",
}


def _c_type_to_decl(com_type: str) -> str:
    return _C_TYPE_DECL_MAP.get(com_type, "uint32")


def render_template(template_name: str, **kwargs) -> str:
    """Render a Jinja2 template from the templates/ directory."""
    env = jinja2.Environment(
        loader=jinja2.FileSystemLoader(str(_TEMPLATE_DIR)),
        keep_trailing_newline=True,
        trim_blocks=True,
        lstrip_blocks=True,
    )
    env.filters["c_type_to_decl"] = _c_type_to_decl
    kwargs.setdefault("date", date.today().isoformat())
    template = env.get_template(template_name)
    return template.render(**kwargs)
