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

- Change the default admin login after first setup.
- Do not expose the web interface directly to the internet.
- HTTP Basic Auth is not encrypted. Use the device only on trusted local networks.
- The admin password is stored as a salted PBKDF2-HMAC-SHA256 hash. Wi-Fi and API credentials must stay readable for the device and are stored as entered.
- Failed logins are throttled per client address (five attempts, then 30 s doubling to 5 min).
- The setup access point uses the default password `pixelclock` until you set your own; it is intended for initial setup or recovery only.
- Before handing the clock on, run the factory reset (web UI or 10 s on the BOOT button). It erases the whole NVS partition including Wi-Fi and API credentials.
