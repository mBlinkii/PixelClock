# Security Policy

## Supported Versions

This project is maintained from the `main` branch. Security fixes should be applied to the latest code.

## Reporting a Vulnerability

If you find a vulnerability, please open a private report if the GitHub repository has private vulnerability reporting enabled. Otherwise contact the maintainer directly before publishing details.

Please include:

- affected version or commit
- steps to reproduce
- expected and actual behavior
- possible impact

## Device Security Notes

- A new clock (no Wi-Fi saved, default password) opens the setup assistant without a login; the assistant requires an own admin password.
- Do not expose the web interface directly to the internet.
- HTTP Basic Auth is not encrypted. Use the device only on trusted local networks.
- The admin password is stored as a salted PBKDF2-HMAC-SHA256 hash. Wi-Fi and API credentials must stay readable for the device and are stored as entered.
- Failed logins are throttled per client address (five attempts, then 30 s doubling to 5 min).
- The setup access point is open until you set your own password; it only runs while no Wi-Fi connection works and is intended for initial setup or recovery.
- Password recovery (`/api/recovery/*`) needs a 6-digit code shown on the matrix. A code is valid for 5 minutes and 5 attempts, new codes need a 30 s gap, and wrong codes count towards the login throttle.
- Before handing the clock on, run the factory reset (web UI or 10 s on the BOOT button). It erases the whole NVS partition including Wi-Fi and API credentials.
