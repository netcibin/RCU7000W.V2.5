let websocket; // Declare websocket variable in a broader scope

function onLoad() {
    var menuContents = document.querySelectorAll('.menu_content');
    menuContents.forEach(function(menuContent) {
        menuContent.style.display = 'none';
    });
    document.getElementById('info').style.display = 'block';

    // WebSocket connection setup
    const port = window.location.port || (window.location.protocol === "https:" ? "443" : "80");
    websocket = new WebSocket(`ws://${window.location.hostname}:${port}/ws`);

    //websocket = new WebSocket(`ws://${window.location.hostname}/ws`);
    websocket.onopen = function(event) {
        console.log('Connection established');
        document.getElementById("ws-status").innerText = "WebSocket: Connected";
        // Keep-alive mechanism (send "ping" every 10s)
        setInterval(() => {
            if (websocket.readyState === WebSocket.OPEN) {
            console.log("Sending ping...");
            websocket.send("ping");
            }
        }, 10000);
    };

    websocket.onclose = function(event) {
        document.getElementById("ws-status").innerText = "WebSocket: Disconnected";
        console.log('Connection died');
    };

    websocket.onerror = function(error) {
        console.error('WebSocket error:', error); // Log the error object
    };

    websocket.onmessage = function(event) {
        if (event.data.startsWith("toggle")) {
            var parts = event.data.split(':');
            var index = parseInt(parts[1]) - 1;
            var state = parts[2] === "1";
            document.getElementById(`state${index + 1}`).innerHTML = state ? "ON" : "OFF";
            document.getElementById(`toggle-btn${index + 1}`).checked = state;
        } else if (event.data.startsWith("{")) {
            const data = JSON.parse(event.data);
            // Update multiple elements based on the received data
            if (data.temperature !== undefined && data.temperature !== null) document.getElementById('temperature').innerHTML = data.temperature;
            if (data.humidity !== undefined && data.humidity !== null) document.getElementById('humidity').innerHTML = data.humidity;
            if (data.presence !== undefined && data.presence !== null) document.getElementById('pr_status').innerHTML = data.presence;
            if (data.key !== undefined && data.key !== null) { document.getElementById('key_status').innerHTML = data.key;
                const keyToggle = document.getElementById('key_toggle');
                // Set toggle switch state based on key value
                keyToggle.checked = data.key === "IN"; // comparison of boolean variable, if the data.key is ON then the switch will be ON otherwise it will be in OFF state.
                }
            if (data.updatekey !== undefined && data.updatekey !== null) document.getElementById('update_key').innerHTML = data.updatekey;
            if (data.wifistatus !== undefined && data.wifistatus !== null) document.getElementById('wifi_status').innerHTML = data.wifistatus;
            if (data.ipaddress !== undefined && data.ipaddress !== null) document.getElementById('ip_address').innerHTML = data.ipaddress;
            if (data.wifisignal !== undefined && data.wifisignal !== null) document.getElementById('wifi_signal').innerHTML = data.wifisignal;
            if (data.acstatus !== undefined && data.acstatus !== null) document.getElementById('ac_status').innerHTML = data.acstatus;
            if (data.setpoint !== undefined && data.setpoint !== null) document.getElementById('set_temp').innerHTML = data.setpoint;
            if (data.fanspeed !== undefined && data.fanspeed !== null) document.getElementById('fan_speed').innerHTML = data.fanspeed;
            if (data.DNDStatus !== undefined && data.DNDStatus !== null) { document.getElementById('dnd_status').innerHTML = data.DNDStatus;
                const dndToggle = document.getElementById('dnd_toggle');
                dndToggle.checked = data.DNDStatus === "ON";
                }
            if (data.MURStatus !== undefined && data.MURStatus !== null) {document.getElementById('mur_status').innerHTML = data.MURStatus;
                const murToggle = document.getElementById('mur_toggle');
                murToggle.checked = data.MURStatus === "ON";
                }
            if (data.MASStatus !== undefined && data.MASStatus !== null) {document.getElementById('master_status').innerHTML = data.MASStatus;
                const masToggle = document.getElementById('master_toggle');
                masToggle.checked = data.MASStatus === "ON";
                }
            if (data.mqttstatus !== undefined && data.mqttstatus !== null) document.getElementById('mqtt-status').innerHTML = data.mqttstatus;
        }
                // If server sends a ping, respond with pong
        else if (event.data === "ping") {
                console.log("Received ping, sending pong...");
                websocket.send("pong");
            }
    };

    // Attach event listeners for toggle buttons in a loop
    for (let i = 1; i <= 14; i++) {
        const toggleButton = document.getElementById(`toggle-btn${i}`);
        if (toggleButton) {
            toggleButton.addEventListener('change', function() {
                toggleRly(i);
            });
        }
    }
}

function toggleRly(index) {
    websocket.send(`toggle:${index}`);
}

function restartRCU() {
    // Send the restart command over WebSocket
    websocket.send('restart'); // You can modify the command if needed
    alert('Restarting RCU.');
}

function updateSetpoint() {
    const setpoint = document.getElementById('setpointInput').value;
    if (setpoint) {
        websocket.send(`setpoint:${setpoint}`);
        alert(`Setpoint updated to ${setpoint}`);
    } else {
        alert("Please enter a valid setpoint value.");
    }
}

function updateDefaultSetpoint() {
    const defaultsetpoint = document.getElementById('defaultsetpointInput').value;
    if (defaultsetpoint) {
        websocket.send(`defaultsetpoint:${defaultsetpoint}`);
        const feedback = document.getElementById('defaultSetMsg');
        feedback.innerHTML = `Setpoint updated to ${setpoint}`;
        feedback.style.display = 'block';
        setTimeout(() => feedback.style.display = 'none', 3000);
    } else {
        const feedback = document.getElementById('defaultSetMsg');
        feedback.innerHTML = `Please enter valid setpoint`;
        feedback.style.display = 'block';
        setTimeout(() => feedback.style.display = 'none', 3000);
    }
}

function MobileMenuChange(menu) {
    menu.classList.toggle('open');
    var tab = document.getElementById('content');
    tab.style.display = tab.style.display === 'none' ? 'block' : 'none';
    // Toggle visibility of footer
    var footer = document.querySelector('footer');
    footer.style.display = footer.style.display === 'none' ? 'block' : 'none';
}

function changeDisplay(menu_item) {
    var menuContents = document.querySelectorAll('.menu_content');
    menuContents.forEach(function(menuContent) {
        menuContent.style.display = 'none';
    });
    document.getElementById(menu_item).style.display = 'block';
}

function toggleKeyTag(toggleElement) {
    const state = toggleElement.checked ? "1" : "0"; // Convert ON/OFF to 1/0
    websocket.send(`keytagStatus:${state}`); // Send the state update via WebSocket
    //document.getElementById('key_status').innerHTML = state === "1" ? "On" : "Off"; // Update displayed status
}

function toggleDnd(toggleElement) {
    const state = toggleElement.checked ? "1" : "0"; // Convert ON/OFF to 1/0
    websocket.send(`dndstatus:${state}`); // Send the state update via WebSocket
}

function toggleMur(toggleElement) {
    const state = toggleElement.checked ? "1" : "0"; // Convert ON/OFF to 1/0
    websocket.send(`murstatus:${state}`); // Send the state update via WebSocket
}

function toggleMaster(toggleElement) {
    const state = toggleElement.checked ? "1" : "0"; // Convert ON/OFF to 1/0
    websocket.send(`masterstatus:${state}`); // Send the state update via WebSocket
}

function scanNetworks() {
    document.getElementById("loading").style.display = "inline";
    fetch('/scan')
        .then(response => response.json())
        .then(data => {
            let networks = document.getElementById("networks");
            networks.innerHTML = "<option value=''>-- Select Network --</option>";
            data.forEach(ssid => {
                let option = document.createElement("option");
                option.value = ssid;
                option.textContent = ssid;
                networks.appendChild(option);
            });
            document.getElementById("loading").style.display = "none";
        });
}

function selectNetwork() {
    let selectedSSID = document.getElementById("networks").value;
    document.getElementById("ssidInput").value = selectedSSID;  // Auto-fill SSID input
}

function saveWiFi() {
    let ssid = document.getElementById("ssidInput").value;
    let password = document.getElementById("password").value;
    fetch('/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `ssid=${ssid}&password=${password}`
    })
    .then(response => response.text())
    .then(alert);
}

function saveMqttSettings() {
    const server = document.getElementById('server').value;
    const port = document.getElementById('port').value;
    const user = document.getElementById('username').value;
    const pass = document.getElementById('mqttpassword').value;

    if (!server || !port) {
        document.getElementById('status').innerText = "Server & Port required!";
        return;
    }

    const mqttdata = {
        server, port, user, pass
    };

    console.log("🔵 Sending MQTT data:", mqttdata); // ✅ Check if password is included

    if (websocket.readyState === WebSocket.OPEN) {
        websocket.send("MQTT:" + JSON.stringify(mqttdata));
        document.getElementById('status').innerText = "Settings Saved!";
    } else {
        document.getElementById('status').innerText = "WebSocket Not Connected!";
    }
}