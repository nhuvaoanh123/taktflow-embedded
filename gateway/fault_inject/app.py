"""Taktflow Fault Injection API — FastAPI server for triggering CAN
fault scenarios during demo.

Endpoints:
    POST /api/fault/scenario/{name}  — trigger a scenario by name
    POST /api/fault/reset            — reset all actuators to safe idle
    GET  /api/fault/scenarios        — list available scenarios
    GET  /api/fault/health           — health check

Runs on FAULT_PORT (default 8091).
"""

import logging
import os

import paho.mqtt.client as paho_mqtt
import uvicorn
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware

from .scenarios import SCENARIOS, reset as reset_scenario, set_mqtt_client

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [FAULT] %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("fault_inject")

# MQTT client for publishing reset/command messages
_mqtt_client: paho_mqtt.Client | None = None


def _init_mqtt() -> paho_mqtt.Client:
    """Initialize MQTT client for fault-inject command publishing."""
    host = os.environ.get("MQTT_HOST", "localhost")
    port = int(os.environ.get("MQTT_PORT", "1883"))
    client = paho_mqtt.Client(
        paho_mqtt.CallbackAPIVersion.VERSION2,
        client_id="taktflow-fault-inject",
    )
    client.connect_async(host, port, keepalive=30)
    client.loop_start()
    log.info("MQTT client connecting to %s:%d", host, port)
    return client


app = FastAPI(
    title="Taktflow Fault Injection API",
    description="Trigger CAN fault scenarios for the Taktflow embedded demo.",
    version="1.0.0",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "https://taktflow-systems.com",
        "https://www.taktflow-systems.com",
        "http://localhost:3000",
    ],
    allow_credentials=True,
    allow_methods=["GET", "POST"],
    allow_headers=["*"],
)


@app.on_event("startup")
def _on_startup():
    global _mqtt_client
    _mqtt_client = _init_mqtt()
    set_mqtt_client(_mqtt_client)


@app.post("/api/fault/scenario/{name}")
def trigger_scenario(name: str):
    """Trigger a fault scenario by name."""
    entry = SCENARIOS.get(name)
    if entry is None:
        raise HTTPException(
            status_code=404,
            detail=f"Unknown scenario '{name}'.  "
                   f"Available: {', '.join(SCENARIOS.keys())}",
        )
    log.info("Triggering scenario: %s", name)
    try:
        result = entry["fn"]()
    except Exception as exc:
        log.error("Scenario '%s' failed: %s", name, exc)
        raise HTTPException(
            status_code=500,
            detail=f"Scenario '{name}' failed: {exc}",
        ) from exc
    log.info("Scenario '%s' complete: %s", name, result)
    return {"scenario": name, "result": result}


@app.post("/api/fault/reset")
def reset_all():
    """Reset all actuators to safe idle state."""
    log.info("Resetting all actuators")
    try:
        result = reset_scenario()
    except Exception as exc:
        log.error("Reset failed: %s", exc)
        raise HTTPException(
            status_code=500,
            detail=f"Reset failed: {exc}",
        ) from exc
    log.info("Reset complete: %s", result)
    return {"result": result}


@app.get("/api/fault/scenarios")
def list_scenarios():
    """List all available fault scenarios with descriptions."""
    return {
        "scenarios": {
            name: entry["description"]
            for name, entry in SCENARIOS.items()
        }
    }


@app.get("/api/fault/health")
def health_check():
    """Health check endpoint."""
    return {
        "status": "ok",
        "service": "fault_inject",
        "can_channel": os.environ.get("CAN_CHANNEL", "vcan0"),
    }


def main():
    port = int(os.environ.get("FAULT_PORT", "8091"))
    log.info("Starting fault injection API on port %d", port)
    uvicorn.run(
        "fault_inject.app:app",
        host="0.0.0.0",
        port=port,
        log_level="info",
    )


if __name__ == "__main__":
    main()
