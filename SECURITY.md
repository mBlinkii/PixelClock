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
- The setup access point uses a fixed default password and is intended for initial setup or recovery only.
