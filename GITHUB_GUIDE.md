# GitHub Publishing Guide

## Before Publishing

1. Ensure all sensitive data is removed from the repository
2. Verify that `config.json` is in `.gitignore` (already configured)
3. Verify that `src/core/telegram_config.hpp` is in `.gitignore` (already configured)
4. Only `config_template.json` should be committed (no real credentials)

## Files to Exclude

The `.gitignore` file already excludes:
- `build/` - Build artifacts
- `*.exe`, `*.dll`, `*.obj`, `*.pdb` - Compiled binaries
- `config.json` - Your personal configuration
- `src/core/telegram_config.hpp` - Generated obfuscated config
- `tmp/` - Temporary extraction folder

## Publishing Steps

### 1. Initialize Git Repository

```bash
git init
git add .
git commit -m "Initial commit"
```

### 2. Create GitHub Repository

1. Go to https://github.com/new
2. Create a new repository (e.g., "chrome-abe-extractor")
3. Choose visibility: Public or Private
4. Do NOT initialize with README (we already have one)

### 3. Add Remote and Push

```bash
git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO_NAME.git
git branch -M main
git push -u origin main
```

### 4. Verify Repository

Check that these files are present:
- README.md
- BUILD.md
- CHANGELOG.md
- LICENSE
- config_template.json
- generate_config.py
- make.bat
- src/ (all source files)
- libs/ (SQLite library)

Check that these files are NOT present:
- config.json (your credentials)
- src/core/telegram_config.hpp (generated config)
- svchost.exe (compiled binary)
- build/ folder

## Repository Description

Suggested description for GitHub:
```
Chrome App-Bound Encryption data extractor with Telegram exfiltration. 
Educational tool for security research and penetration testing.
```

## Topics/Tags

Suggested tags:
- security-research
- penetration-testing
- chrome
- app-bound-encryption
- telegram-bot
- windows
- cpp

## Important Notes

1. Add a clear disclaimer in README.md about legal and ethical use
2. This tool is for educational and authorized security testing only
3. Unauthorized use may violate laws and regulations
4. Users are responsible for compliance with applicable laws

## License

The project uses MIT License, which allows:
- Commercial use
- Modification
- Distribution
- Private use

But requires:
- License and copyright notice inclusion
- No liability or warranty

## Credits

The README.md already includes credits to the original repository:
https://github.com/xaitax/Chrome-App-Bound-Encryption-Decryption/

## After Publishing

1. Test cloning the repository in a clean environment
2. Follow BUILD.md instructions to verify they work
3. Consider adding GitHub Actions for automated builds (optional)
4. Monitor issues and pull requests
5. Keep dependencies updated

## Security Considerations

- Never commit real Telegram credentials
- Never commit compiled binaries with embedded credentials
- Review all commits before pushing
- Use `.gitignore` to prevent accidental credential leaks
- Consider using GitHub's secret scanning alerts
