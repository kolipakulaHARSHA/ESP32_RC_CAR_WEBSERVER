// HTTP + WebSocket server: serves the web UI from LittleFS and forwards
// drive commands received over the WebSocket to the drive layer.
#pragma once

namespace web {

// Mounts LittleFS, registers routes and starts the server.
// Returns false if the web UI files could not be mounted (server still starts
// and reports the problem to the browser).
bool begin();

// Periodic housekeeping; call from the main loop.
void loop();

}  // namespace web
