@echo off
title Taktflow CAN Bus Monitor
cd /d "%~dp0.."
python -m private.can-monitor.main %*
