'use strict';

const STOP_COMMAND = 'off';
const HEARTBEAT_COMMAND = 'hb';
const HEARTBEAT_INTERVAL_MS = 100;
const RECONNECT_DELAY_MS = 1000;

const statusEl = document.getElementById('status');
const stateEl = document.getElementById('vehicle-state');
const speedSlider = document.getElementById('speed-slider');
const speedDisplay = document.getElementById('speed-display');

let socket = null;
let activeCommand = null;
let heartbeatTimer = null;

function setConnectionStatus(text, connected) {
  statusEl.textContent = text;
  statusEl.classList.toggle('ok', connected);
}

function updateVehicleState(stateName) {
  stateEl.textContent = stateName;
  stateEl.className = 'state-badge';
  const lower = stateName.toLowerCase();
  if (lower === 'armed') stateEl.classList.add('armed');
  else if (lower === 'driving') stateEl.classList.add('driving');
  else if (lower === 'failsafe') stateEl.classList.add('failsafe');
}

function send(command) {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(command);
  }
}

function startHeartbeatLoop() {
  stopHeartbeatLoop();
  heartbeatTimer = setInterval(() => {
    if (activeCommand) {
      // Refresh active driving command continuously to feed watchdog & fail-safe timer
      send(activeCommand);
    } else {
      // Send keepalive heartbeat when idle/stationary
      send(HEARTBEAT_COMMAND);
    }
  }, HEARTBEAT_INTERVAL_MS);
}

function stopHeartbeatLoop() {
  if (heartbeatTimer) {
    clearInterval(heartbeatTimer);
    heartbeatTimer = null;
  }
}

function connect() {
  socket = new WebSocket(`ws://${location.host}/ws`);

  socket.addEventListener('open', () => {
    setConnectionStatus('Connected', true);
    startHeartbeatLoop();
  });

  socket.addEventListener('message', (event) => {
    const text = event.data;
    if (typeof text === 'string' && text.startsWith('state:')) {
      const state = text.substring(6).trim();
      updateVehicleState(state);
    }
  });

  socket.addEventListener('close', () => {
    setConnectionStatus('Reconnecting\u2026', false);
    updateVehicleState('DISCONNECTED');
    stopHeartbeatLoop();
    setTimeout(connect, RECONNECT_DELAY_MS);
  });

  socket.addEventListener('error', () => {
    socket.close();
  });
}

// Hold-to-drive interaction with Pointer Events (pointerdown, lostpointercapture)
document.querySelectorAll('[data-cmd]').forEach((button) => {
  const command = button.dataset.cmd;

  button.addEventListener('pointerdown', (event) => {
    event.preventDefault();
    button.setPointerCapture(event.pointerId);
    button.classList.add('active');
    activeCommand = command;
    send(command);
  });

  button.addEventListener('lostpointercapture', () => {
    button.classList.remove('active');
    if (activeCommand === command) {
      activeCommand = null;
    }
    send(STOP_COMMAND);
  });

  button.addEventListener('contextmenu', (event) => event.preventDefault());
});

// Speed slider input listener
speedSlider.addEventListener('input', () => {
  const val = speedSlider.value;
  speedDisplay.textContent = `${val} / 255`;
  send(`speed:${val}`);
});

// Safety halts on tab loss or window blur
window.addEventListener('blur', () => {
  activeCommand = null;
  send(STOP_COMMAND);
});

document.addEventListener('visibilitychange', () => {
  if (document.hidden) {
    activeCommand = null;
    send(STOP_COMMAND);
  }
});

// Initial connection
connect();
