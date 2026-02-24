/* ============================================================
   Taktflow SIL Dashboard — app.js
   WebSocket telemetry, fault injection, controller lock, SAP QM
   ============================================================ */

(function () {
    "use strict";

    // ── Client ID (persisted in sessionStorage) ──────────────
    var CLIENT_ID = sessionStorage.getItem("taktflow_client_id");
    if (!CLIENT_ID) {
        CLIENT_ID = "web-" + Math.random().toString(36).slice(2, 10);
        sessionStorage.setItem("taktflow_client_id", CLIENT_ID);
    }

    // ── DOM references ───────────────────────────────────────
    var $id = document.getElementById.bind(document);

    var dom = {
        wsStatus:       $id("ws-status"),
        uptime:         $id("uptime"),
        canRate:        $id("can-rate"),

        vehicleState:   $id("vehicle-state"),
        vehicleFaults:  $id("vehicle-faults"),
        torqueLimit:    $id("vehicle-torque-limit"),
        speedLimit:     $id("vehicle-speed-limit"),

        motorRpm:       $id("motor-rpm"),
        motorCurrent:   $id("motor-current"),
        motorTemp:      $id("motor-temp"),
        motorTemp2:     $id("motor-temp2"),
        motorDuty:      $id("motor-duty"),
        motorFaults:    $id("motor-faults"),

        steerActual:    $id("steer-actual"),
        steerCmd:       $id("steer-cmd"),
        steerServo:     $id("steer-servo"),
        steerFault:     $id("steer-fault"),

        brakePos:       $id("brake-pos"),
        brakeCmd:       $id("brake-cmd"),
        brakeServo:     $id("brake-servo"),
        brakeFault:     $id("brake-fault"),

        battVoltage:    $id("batt-voltage"),
        battSoc:        $id("batt-soc"),
        battStatus:     $id("batt-status"),
        battBar:        $id("batt-bar"),

        lidarDist:      $id("lidar-dist"),
        lidarZone:      $id("lidar-zone"),
        lidarSignal:    $id("lidar-signal"),

        hbCvc:          $id("hb-cvc"),
        hbFzc:          $id("hb-fzc"),
        hbRzc:          $id("hb-rzc"),

        anomalyScore:   $id("anomaly-score"),
        anomalyAlert:   $id("anomaly-alert"),
        anomalyBar:     $id("anomaly-bar"),

        lockStatus:     $id("lock-status"),
        lockRemaining:  $id("lock-remaining"),
        btnAcquire:     $id("btn-acquire"),
        btnRelease:     $id("btn-release"),
        feedback:       $id("control-feedback"),

        canLog:         $id("can-log"),
        eventLog:       $id("event-log"),

        sapTbody:       $id("sap-tbody"),
        sapEmpty:       $id("sap-empty"),
    };

    // ── Constants ────────────────────────────────────────────
    var MAX_LOG_ENTRIES = 200;
    var SAP_POLL_INTERVAL = 5000;
    var RECONNECT_BASE_MS = 1000;
    var RECONNECT_MAX_MS = 30000;

    var STATE_NAMES = ["INIT", "RUN", "DEGRADED", "LIMP", "SAFE_STOP", "SHUTDOWN"];
    var STATE_CLASSES = ["state-init", "state-run", "state-degraded", "state-limp", "state-safe-stop", "state-shutdown"];
    var BATT_STATUS = { 0: "CRITICAL", 1: "LOW", 2: "NOMINAL", 3: "OV_WARN", 4: "OV_CRITICAL" };

    // ── State ────────────────────────────────────────────────
    var ws = null;
    var reconnectDelay = RECONNECT_BASE_MS;
    var reconnectTimer = null;
    var canLogCount = 0;
    var eventLogCount = 0;
    var lastCanLogTs = 0;
    var lastEventTs = 0;
    var sapPollTimer = null;

    // ── Helpers ──────────────────────────────────────────────

    function formatTime(sec) {
        var h = Math.floor(sec / 3600);
        var m = Math.floor((sec % 3600) / 60);
        var s = Math.floor(sec % 60);
        return pad2(h) + ":" + pad2(m) + ":" + pad2(s);
    }

    function pad2(n) { return n < 10 ? "0" + n : "" + n; }

    function tsToTime(ts) {
        var d = new Date(ts * 1000);
        return pad2(d.getHours()) + ":" + pad2(d.getMinutes()) + ":" + pad2(d.getSeconds());
    }

    function setTextContent(el, text) {
        if (el && el.textContent !== text) el.textContent = text;
    }

    function setInnerHTML(el, html) {
        if (el) el.innerHTML = html;
    }

    function showFeedback(msg, cls) {
        dom.feedback.textContent = msg;
        dom.feedback.className = "control-feedback " + (cls || "");
    }

    // ── WebSocket ────────────────────────────────────────────

    function connect() {
        if (ws && (ws.readyState === WebSocket.CONNECTING || ws.readyState === WebSocket.OPEN)) return;

        var proto = location.protocol === "https:" ? "wss:" : "ws:";
        ws = new WebSocket(proto + "//" + location.host + "/ws/telemetry");

        setStatus("reconnecting", "Connecting...");

        ws.onopen = function () {
            reconnectDelay = RECONNECT_BASE_MS;
            setStatus("connected", "Connected");
        };

        ws.onmessage = function (e) {
            try {
                var data = JSON.parse(e.data);
                updateDashboard(data);
            } catch (_) { /* ignore malformed frames */ }
        };

        ws.onclose = function () {
            setStatus("disconnected", "Disconnected");
            scheduleReconnect();
        };

        ws.onerror = function () {
            ws.close();
        };
    }

    function scheduleReconnect() {
        if (reconnectTimer) return;
        setStatus("reconnecting", "Reconnecting...");
        reconnectTimer = setTimeout(function () {
            reconnectTimer = null;
            reconnectDelay = Math.min(reconnectDelay * 2, RECONNECT_MAX_MS);
            connect();
        }, reconnectDelay);
    }

    function setStatus(cls, text) {
        dom.wsStatus.className = "status-badge " + cls;
        setTextContent(dom.wsStatus, text);
    }

    // ── Dashboard update ─────────────────────────────────────

    function updateDashboard(d) {
        // Stats
        if (d.stats) {
            setTextContent(dom.uptime, formatTime(d.stats.uptime_sec || 0));
            setTextContent(dom.canRate, (d.stats.can_msgs_sec || 0) + " msg/s");
        }

        // Vehicle
        if (d.vehicle) {
            var si = d.vehicle.state || 0;
            var sName = d.vehicle.state_name || STATE_NAMES[si] || "UNKNOWN";
            setTextContent(dom.vehicleState, sName);
            dom.vehicleState.className = "metric-value state-badge " + (STATE_CLASSES[si] || "");
            setTextContent(dom.vehicleFaults, "0x" + ((d.vehicle.fault_mask || 0) >>> 0).toString(16).toUpperCase().padStart(4, "0"));
            setInnerHTML(dom.torqueLimit, '<span class="num">' + (d.vehicle.torque_limit != null ? d.vehicle.torque_limit : "--") + '</span>%');
            setInnerHTML(dom.speedLimit, '<span class="num">' + (d.vehicle.speed_limit != null ? d.vehicle.speed_limit : "--") + '</span>%');
        }

        // Motor
        if (d.motor) {
            setInnerHTML(dom.motorRpm, '<span class="num">' + (d.motor.rpm || 0) + "</span>");
            setInnerHTML(dom.motorCurrent, '<span class="num">' + (d.motor.current_ma || 0) + "</span> mA");
            var tempC = d.motor.temp_c || 0;
            var tempCls = tempC > 80 ? "val-fault" : tempC > 60 ? "val-warn" : "";
            setInnerHTML(dom.motorTemp, '<span class="num ' + tempCls + '">' + tempC + "</span>&deg;C");
            setInnerHTML(dom.motorTemp2, '<span class="num">' + (d.motor.temp2_c || 0) + "</span>&deg;C");
            setInnerHTML(dom.motorDuty, '<span class="num">' + (d.motor.duty_pct || 0) + "</span>%");
            var mf = d.motor.faults || 0;
            dom.motorFaults.textContent = mf;
            dom.motorFaults.className = "metric-value" + (mf > 0 ? " val-fault" : "");
        }

        // Steering
        if (d.steering) {
            setInnerHTML(dom.steerActual, '<span class="num">' + (d.steering.actual_deg != null ? d.steering.actual_deg.toFixed(1) : "--") + "</span>&deg;");
            setInnerHTML(dom.steerCmd, '<span class="num">' + (d.steering.commanded_deg != null ? d.steering.commanded_deg.toFixed(1) : "--") + "</span>&deg;");
            setInnerHTML(dom.steerServo, '<span class="num">' + (d.steering.servo_ma || 0) + "</span> mA");
            var sf = d.steering.fault || 0;
            setTextContent(dom.steerFault, sf ? "FAULT" : "OK");
            dom.steerFault.className = "metric-value fault-indicator " + (sf ? "fault" : "ok");
        }

        // Brake
        if (d.brake) {
            setInnerHTML(dom.brakePos, '<span class="num">' + (d.brake.position_pct || 0) + "</span>%");
            setInnerHTML(dom.brakeCmd, '<span class="num">' + (d.brake.commanded_pct || 0) + "</span>%");
            setInnerHTML(dom.brakeServo, '<span class="num">' + (d.brake.servo_ma || 0) + "</span> mA");
            var bf = d.brake.fault || 0;
            setTextContent(dom.brakeFault, bf ? "FAULT" : "OK");
            dom.brakeFault.className = "metric-value fault-indicator " + (bf ? "fault" : "ok");
        }

        // Battery
        if (d.battery) {
            var vMv = d.battery.voltage_mv || 0;
            var vV = (vMv / 1000).toFixed(2);
            setInnerHTML(dom.battVoltage, '<span class="num">' + vV + "</span> V");
            var soc = d.battery.soc_pct != null ? d.battery.soc_pct : 0;
            setInnerHTML(dom.battSoc, '<span class="num">' + soc + "</span>%");
            setTextContent(dom.battStatus, BATT_STATUS[d.battery.status] || "UNKNOWN");
            dom.battBar.style.width = soc + "%";
            dom.battBar.className = "bar" + (soc < 20 ? " bar-low" : "");
        }

        // LiDAR
        if (d.lidar) {
            setInnerHTML(dom.lidarDist, '<span class="num">' + (d.lidar.distance_cm || 0) + "</span> cm");
            setTextContent(dom.lidarZone, d.lidar.zone != null ? d.lidar.zone : "--");
            setInnerHTML(dom.lidarSignal, '<span class="num">' + (d.lidar.signal_strength || 0) + "</span>");
        }

        // Heartbeats
        if (d.heartbeats) {
            setHb(dom.hbCvc, d.heartbeats.cvc);
            setHb(dom.hbFzc, d.heartbeats.fzc);
            setHb(dom.hbRzc, d.heartbeats.rzc);
        }

        // Anomaly
        if (d.anomaly) {
            var score = d.anomaly.score || 0;
            setInnerHTML(dom.anomalyScore, '<span class="num">' + score.toFixed(2) + "</span>");
            var alert = d.anomaly.alert || false;
            setTextContent(dom.anomalyAlert, alert ? "ALERT" : "OK");
            dom.anomalyAlert.className = "metric-value fault-indicator " + (alert ? "fault" : "ok");
            dom.anomalyBar.style.width = Math.min(score * 100, 100) + "%";
            dom.anomalyBar.className = "bar bar-anomaly" + (alert ? " alert" : "");
        }

        // Control lock
        if (d.control) {
            updateLockUI(d.control);
        }

        // CAN log
        if (d.can_log && d.can_log.length) {
            appendCanLog(d.can_log);
        }

        // Events
        if (d.events && d.events.length) {
            appendEvents(d.events);
        }

        // SAP notifications (inline from telemetry)
        if (d.sap_notifications && d.sap_notifications.length) {
            renderSapFromTelemetry(d.sap_notifications);
        }
    }

    function setHb(el, alive) {
        if (!el) return;
        el.className = "hb-dot " + (alive ? "alive" : "dead");
    }

    // ── Lock UI ──────────────────────────────────────────────

    function updateLockUI(ctrl) {
        var locked = ctrl.locked;
        var holder = ctrl.client_id || "";
        var remaining = ctrl.remaining_sec || 0;
        var isMine = locked && holder === CLIENT_ID;

        if (!locked) {
            dom.lockStatus.textContent = "Unlocked";
            dom.lockStatus.className = "lock-badge unlocked";
            dom.lockRemaining.textContent = "";
        } else if (isMine) {
            dom.lockStatus.textContent = "You have control";
            dom.lockStatus.className = "lock-badge mine";
            dom.lockRemaining.textContent = Math.ceil(remaining) + "s remaining";
        } else {
            dom.lockStatus.textContent = "Locked by " + holder;
            dom.lockStatus.className = "lock-badge locked";
            dom.lockRemaining.textContent = Math.ceil(remaining) + "s remaining";
        }
    }

    // ── CAN Log ──────────────────────────────────────────────

    function appendCanLog(entries) {
        var frag = document.createDocumentFragment();
        var newCount = 0;
        for (var i = 0; i < entries.length; i++) {
            var e = entries[i];
            if (e.ts <= lastCanLogTs) continue;
            var div = document.createElement("div");
            div.className = "log-entry";
            var signals = e.signals ? " " + JSON.stringify(e.signals) : "";
            div.innerHTML = '<span class="log-ts">' + tsToTime(e.ts) + "</span>" +
                '<span class="log-msg-name">' + esc(e.msg_name || e.msg_id || "") + "</span>" +
                " [" + esc(e.sender || "") + "]" + esc(signals);
            frag.appendChild(div);
            newCount++;
        }
        if (newCount > 0) {
            dom.canLog.appendChild(frag);
            canLogCount += newCount;
            trimLog(dom.canLog, canLogCount);
            dom.canLog.scrollTop = dom.canLog.scrollHeight;
        }
        if (entries.length > 0) {
            lastCanLogTs = entries[entries.length - 1].ts;
        }
    }

    // ── Event Log ────────────────────────────────────────────

    var EVENT_CLASS_MAP = {
        fault:   "log-event-fault",
        dtc:     "log-event-fault",
        state:   "log-event-state",
        anomaly: "log-event-state",
        sap:     "log-event-sap",
        info:    "log-event-info",
    };

    function appendEvents(entries) {
        var frag = document.createDocumentFragment();
        var newCount = 0;
        for (var i = 0; i < entries.length; i++) {
            var e = entries[i];
            if (e.ts <= lastEventTs) continue;
            var div = document.createElement("div");
            div.className = "log-entry";
            var cls = EVENT_CLASS_MAP[e.type] || "log-event-info";
            div.innerHTML = '<span class="log-ts">' + tsToTime(e.ts) + "</span>" +
                '<span class="' + cls + '">' + esc(e.msg || "") + "</span>";
            frag.appendChild(div);
            newCount++;
        }
        if (newCount > 0) {
            dom.eventLog.appendChild(frag);
            eventLogCount += newCount;
            trimLog(dom.eventLog, eventLogCount);
            dom.eventLog.scrollTop = dom.eventLog.scrollHeight;
        }
        if (entries.length > 0) {
            lastEventTs = entries[entries.length - 1].ts;
        }
    }

    function trimLog(container, count) {
        while (container.childNodes.length > MAX_LOG_ENTRIES) {
            container.removeChild(container.firstChild);
        }
    }

    function esc(s) {
        var d = document.createElement("div");
        d.textContent = s;
        return d.innerHTML;
    }

    // ── SAP QM ───────────────────────────────────────────────

    var sapNotifications = [];

    function renderSapFromTelemetry(notifs) {
        // Merge new notifications by ID
        for (var i = 0; i < notifs.length; i++) {
            var n = notifs[i];
            var found = false;
            for (var j = 0; j < sapNotifications.length; j++) {
                if (sapNotifications[j].notification_id === n.notification_id) {
                    sapNotifications[j] = n;
                    found = true;
                    break;
                }
            }
            if (!found) sapNotifications.push(n);
        }
        renderSapTable();
    }

    function pollSap() {
        fetch("/api/sap/opu/odata/sap/API_QUALITYNOTIFICATION/QualityNotification?$top=20&$orderby=created_at%20desc")
            .then(function (r) { return r.ok ? r.json() : null; })
            .then(function (data) {
                if (data && data.d && data.d.results) {
                    sapNotifications = data.d.results;
                    renderSapTable();
                }
            })
            .catch(function () { /* silent — SAP may not be up yet */ });
    }

    function renderSapTable() {
        if (!sapNotifications.length) {
            dom.sapTbody.innerHTML = "";
            dom.sapEmpty.style.display = "";
            return;
        }
        dom.sapEmpty.style.display = "none";

        var html = "";
        for (var i = 0; i < sapNotifications.length; i++) {
            var n = sapNotifications[i];
            var priClass = "sap-priority-" + (n.priority || "3");
            var statusRaw = (n.status || "").toUpperCase();
            var statusClass = "sap-status sap-status-" + statusRaw.toLowerCase();
            var created = n.created_at ? new Date(n.created_at).toLocaleString() : "--";
            html += "<tr>" +
                "<td>" + esc(n.notification_id || "") + "</td>" +
                "<td>" + esc(n.dtc_code || "") + "</td>" +
                "<td>" + esc(n.description || n.defect_text || "") + "</td>" +
                "<td>" + esc(n.plant || "") + "</td>" +
                '<td class="' + priClass + '">' + esc(n.priority || "") + "</td>" +
                '<td><span class="' + statusClass + '">' + esc(statusRaw) + "</span></td>" +
                "<td>" + esc(created) + "</td>" +
                "</tr>";
        }
        dom.sapTbody.innerHTML = html;
    }

    // ── Fault Injection ──────────────────────────────────────

    function triggerScenario(scenario) {
        disableScenarioButtons(true);
        showFeedback("Triggering " + scenario + "...", "");

        fetch("/api/fault/scenario/" + encodeURIComponent(scenario), {
            method: "POST",
            headers: { "X-Client-Id": CLIENT_ID },
        })
            .then(function (r) {
                if (r.ok) return r.json().then(function (d) { showFeedback(d.result || "OK", "success"); });
                if (r.status === 403) return r.json().then(function (d) { showFeedback(d.detail || "Locked by another user", "error"); });
                if (r.status === 423) return showFeedback("Acquire the lock first", "error");
                showFeedback("Error " + r.status, "error");
            })
            .catch(function (err) { showFeedback("Network error", "error"); })
            .finally(function () { disableScenarioButtons(false); });
    }

    function disableScenarioButtons(disabled) {
        var btns = document.querySelectorAll(".btn-scenario");
        for (var i = 0; i < btns.length; i++) btns[i].disabled = disabled;
    }

    // ── Lock Controls ────────────────────────────────────────

    function acquireLock() {
        fetch("/api/fault/control/acquire", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ client_id: CLIENT_ID }),
        })
            .then(function (r) { return r.json().then(function (d) { return { status: r.status, data: d }; }); })
            .then(function (res) {
                if (res.status === 200) {
                    showFeedback("Lock acquired (" + (res.data.remaining_sec || 120) + "s)", "success");
                } else if (res.status === 409) {
                    showFeedback(res.data.detail || "Another user has control", "error");
                } else {
                    showFeedback("Error " + res.status, "error");
                }
            })
            .catch(function () { showFeedback("Network error", "error"); });
    }

    function releaseLock() {
        fetch("/api/fault/control/release", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ client_id: CLIENT_ID }),
        })
            .then(function (r) { return r.json().then(function (d) { return { status: r.status, data: d }; }); })
            .then(function (res) {
                if (res.status === 200) {
                    showFeedback("Lock released", "success");
                } else {
                    showFeedback(res.data.detail || "Error " + res.status, "error");
                }
            })
            .catch(function () { showFeedback("Network error", "error"); });
    }

    // ── Event Binding ────────────────────────────────────────

    function init() {
        // Scenario buttons
        var btns = document.querySelectorAll(".btn-scenario");
        for (var i = 0; i < btns.length; i++) {
            btns[i].addEventListener("click", function () {
                triggerScenario(this.getAttribute("data-scenario"));
            });
        }

        // Lock buttons
        dom.btnAcquire.addEventListener("click", acquireLock);
        dom.btnRelease.addEventListener("click", releaseLock);

        // WebSocket
        connect();

        // SAP polling
        pollSap();
        sapPollTimer = setInterval(pollSap, SAP_POLL_INTERVAL);
    }

    // ── Start ────────────────────────────────────────────────
    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", init);
    } else {
        init();
    }
})();
