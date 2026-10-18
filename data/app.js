'use strict';

const STOP_COMMAND = 'off';
const RECONNECT_DELAY_MS = 1000;

const statusEl = document.getElementById('status');
let socket = null;

function setStatus(text, connected) {
  statusEl.textContent = text;
  statusEl.classList.toggle('ok', connected);
}

function connect() {
  socket = new WebSocket(`ws://${location.host}/ws`);
  socket.addEventListener('open', () => setStatus('Connected', true));
  socket.addEventListener('close', () => {
    setStatus('Disconnected \u2013 reconnecting\u2026', false);
    setTimeout(connect, RECONNECT_DELAY_MS);
  });
}

function send(command) {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(command);
  }
}

// Hold-to-drive: send the command on press, STOP on release.
// Pointer events cover mouse, touch and pen in one code path. Capturing the
// pointer guarantees we see the release even if the finger slides off the button.
document.querySelectorAll('[data-cmd]').forEach((button) => {
  const command = button.dataset.cmd;

  button.addEventListener('pointerdown', (event) => {
    event.preventDefault();
    button.setPointerCapture(event.pointerId);
    send(command);
  });

  // Fires after pointerup/pointercancel while captured.
  button.addEventListener('lostpointercapture', () => send(STOP_COMMAND));

  button.addEventListener('contextmenu', (event) => event.preventDefault());
});

// Stop if the page loses focus (tab switch, notification, screen lock).
window.addEventListener('blur', () => send(STOP_COMMAND));
document.addEventListener('visibilitychange', () => {
  if (document.hidden) send(STOP_COMMAND);
});

connect();
