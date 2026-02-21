# RTE — Runtime Environment

Signal buffer and port connections between SWCs and BSW.

| Feature | Description |
|---------|-------------|
| Rte_Read | SWC reads signal from buffer |
| Rte_Write | SWC writes signal to buffer |
| Runnable scheduling | Which SWC runs at which tick rate |
| Port connections | Compile-time per-ECU configuration |

Per-ECU configs: firmware/{ecu}/cfg/Rte_Cfg_{Ecu}.c

Phase 5 deliverable.
