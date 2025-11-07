
# Smart Home IoT Security Platform

## 🏠 Project Overview  
This project involves the deployment of a secure local IoT network that ensures reliable communication between sensors, gateways and the cloud via the ThingsBoard Cloud platform.  
Key elements include:  
- A secure MQTT/TLS communication channel between devices and cloud.  
- Strong device authentication using access tokens and TLS/mTLS certificates, to prevent unauthorized connections.  
- Integration of a OpenVPN tunnel between the IoT network and the ThingsBoard Cloud for secure back-haul communication.  
- Automation of alerts and cloud-to-device notifications for remote actuation.  
- Real-time telemetry visualization on dashboards within ThingsBoard.

---

## 🚀 Features  
- **Secure MQTT over TLS/mTLS** — Devices connect via MQTT with encryption and mutual-authentication.  
- **Device access control** — Use of tokens and certificates to restrict device onboarding and operations.  
- **VPN Back-haul** — An OpenVPN tunnel ensures the local IoT domain communicates securely with the cloud platform.  
- **Real-time monitoring & actuation** — Telemetry from sensors is visualized, and remote commands/alerts can be sent to devices.  
- **Dashboarding & Alerts** — ThingsBoard dashboards display live data; alerts trigger based on defined thresholds or events.

---
<br><br>
![](https://github.com/Douaa03/Douaa03/blob/main/schéma.png)

## 🔧 Technologies / Stack  
- Device layer: ESP32/other microcontrollers
- Communication: MQTT protocol over TLS/mTLS for secure transport  
- Network/Infrastructure: Local LAN/WAN, gateways, VLANs, firewall rules, VPN tunnel (OpenVPN)  
- Cloud & Platform: ThingsBoard Cloud for device management, visualization, telemetry, alerting :contentReference[oaicite:2]{index=2}  
- Security: Certificate-based authentication, token authentication, strong access control and network segmentation  
- Visualization: ThingsBoard dashboards for telemetry and remote control

---

## 🛠️ Getting Started  
### Prerequisites  
- A ThingsBoard Cloud account or ThingsBoard on-premises deployment  
- MQTT broker configured for TLS/mTLS or using ThingsBoard’s built-in MQTT with SSL 
- OpenVPN server configured to link the IoT network and cloud platform  
- IoT devices capable of MQTT over TLS and certificate handling  


