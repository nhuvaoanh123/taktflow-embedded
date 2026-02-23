#!/usr/bin/env python3
"""
Train an Isolation Forest model on synthetic normal CAN telemetry data.

Features (5):
  - motor_current_mean   (500–3000 mA, normal operation)
  - motor_current_std    (50–200 mA)
  - motor_temp           (25–70 °C, below overtemp threshold)
  - rpm                  (0–3500, normal operating range)
  - battery_voltage      (11500–13000 mV)

Outputs:
  anomaly_model.pkl  — fitted IsolationForest
  scaler.pkl         — fitted StandardScaler
"""

from __future__ import annotations

import os
import logging
from pathlib import Path

import numpy as np
import joblib
from sklearn.ensemble import IsolationForest
from sklearn.preprocessing import StandardScaler

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
FEATURE_NAMES = [
    "motor_current_mean",
    "motor_current_std",
    "motor_temp",
    "rpm",
    "battery_voltage",
]

N_SAMPLES = 5000
RANDOM_SEED = 42

MODEL_DIR = Path(__file__).resolve().parent
MODEL_PATH = MODEL_DIR / "anomaly_model.pkl"
SCALER_PATH = MODEL_DIR / "scaler.pkl"


# ---------------------------------------------------------------------------
# Synthetic data generation
# ---------------------------------------------------------------------------
def generate_normal_data(n_samples: int = N_SAMPLES, seed: int = RANDOM_SEED) -> np.ndarray:
    """Generate synthetic *normal* CAN telemetry features.

    Each feature is sampled uniformly within its healthy operating range.
    """
    rng = np.random.default_rng(seed)

    motor_current_mean = rng.uniform(500, 3000, size=n_samples)
    motor_current_std = rng.uniform(50, 200, size=n_samples)
    motor_temp = rng.uniform(25, 70, size=n_samples)
    rpm = rng.uniform(0, 3500, size=n_samples)
    battery_voltage = rng.uniform(11500, 13000, size=n_samples)

    data = np.column_stack([
        motor_current_mean,
        motor_current_std,
        motor_temp,
        rpm,
        battery_voltage,
    ])
    return data


# ---------------------------------------------------------------------------
# Training
# ---------------------------------------------------------------------------
def train_model(
    n_samples: int = N_SAMPLES,
    contamination: float = 0.05,
    n_estimators: int = 100,
    random_state: int = RANDOM_SEED,
) -> tuple[IsolationForest, StandardScaler]:
    """Train IsolationForest on synthetic normal data and return (model, scaler)."""

    logger.info("Generating %d synthetic normal samples …", n_samples)
    X = generate_normal_data(n_samples=n_samples, seed=random_state)

    logger.info("Fitting StandardScaler …")
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)

    logger.info(
        "Training IsolationForest (n_estimators=%d, contamination=%.2f) …",
        n_estimators,
        contamination,
    )
    model = IsolationForest(
        n_estimators=n_estimators,
        contamination=contamination,
        random_state=random_state,
    )
    model.fit(X_scaled)

    return model, scaler


def save_model(
    model: IsolationForest,
    scaler: StandardScaler,
    model_path: Path = MODEL_PATH,
    scaler_path: Path = SCALER_PATH,
) -> None:
    """Persist model and scaler to disk via joblib."""
    os.makedirs(model_path.parent, exist_ok=True)
    joblib.dump(model, model_path)
    joblib.dump(scaler, scaler_path)
    logger.info("Saved model  → %s", model_path)
    logger.info("Saved scaler → %s", scaler_path)


# ---------------------------------------------------------------------------
# CLI entry-point
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(message)s",
    )
    model, scaler = train_model()
    save_model(model, scaler)
    logger.info("Training complete.")
