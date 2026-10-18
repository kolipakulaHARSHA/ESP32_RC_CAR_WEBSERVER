// Wi-Fi station (client) connection.
#pragma once

namespace net {

// Connects to the access point defined in include/secrets.h.
// Blocks until connected, then logs the URL of the control page.
void connectWifi();

}  // namespace net
