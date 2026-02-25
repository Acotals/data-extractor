#!/usr/bin/env python3
import json
import sys

def xor_encode(text, key=0xAA):
    return ', '.join(f'0x{ord(c) ^ key:02X}' for c in text)

def generate_config_header(bot_token, channel_id):
    bot_token_enc = xor_encode(bot_token)
    channel_id_enc = xor_encode(channel_id)
    
    header = f'''#pragma once

static const char TELEGRAM_BOT_TOKEN_ENC[] = {{ {bot_token_enc} }};
static const char TELEGRAM_CHANNEL_ID_ENC[] = {{ {channel_id_enc} }};
static const size_t TELEGRAM_BOT_TOKEN_LEN = {len(bot_token)};
static const size_t TELEGRAM_CHANNEL_ID_LEN = {len(channel_id)};
'''
    
    return header

def main():
    if len(sys.argv) != 2:
        print("Usage: python generate_config.py config.json")
        print("\nConfig file format:")
        print('{')
        print('  "telegram": {')
        print('    "bot_token": "YOUR_BOT_TOKEN",')
        print('    "channel_id": "YOUR_CHANNEL_ID"')
        print('  }')
        print('}')
        sys.exit(1)
    
    config_file = sys.argv[1]
    
    try:
        with open(config_file, 'r') as f:
            config = json.load(f)
        
        bot_token = config['telegram']['bot_token']
        channel_id = config['telegram']['channel_id']
        
        if bot_token == "YOUR_BOT_TOKEN_HERE" or channel_id == "YOUR_CHANNEL_ID_HERE":
            print("Error: Please configure your Telegram credentials in the config file!")
            sys.exit(1)
        
        header_content = generate_config_header(bot_token, channel_id)
        
        output_file = 'src/core/telegram_config.hpp'
        with open(output_file, 'w') as f:
            f.write(header_content)
        
        print(f"✓ Configuration generated: {output_file}")
        print(f"✓ Bot token length: {len(bot_token)} chars")
        print(f"✓ Channel ID length: {len(channel_id)} chars")
        print("\nYou can now compile the project with: make.bat")
        
    except FileNotFoundError:
        print(f"Error: Config file '{config_file}' not found!")
        print("Create it from config_template.json")
        sys.exit(1)
    except KeyError as e:
        print(f"Error: Missing key in config file: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
