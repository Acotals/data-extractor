# Changelog

## [1.0.0] - 2026

### Added
- Initial release (based on xaitax's Chrome-App-Bound-Encryption-Decryption)
- App-Bound Encryption decryption using COM IElevator
- Support for Chrome, Edge, Brave, and Avast browsers
- Automatic data extraction (cookies, passwords, credit cards, tokens)
- Telegram upload with ChaCha20 encryption
- Process injection with syscall evasion
- Silent execution (no console window)
- Automatic cleanup of temporary files
- XOR string obfuscation
- Named pipe IPC between injector and payload

### Technical Features
- Direct syscalls for API evasion
- ChaCha20 payload encryption
- AES-GCM data decryption
- SQLite database parsing
- COM interface hijacking
- WinHTTP for network communication
