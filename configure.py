#!/usr/bin/env python3
"""
SmartPong Setup & Local Configuration Utility
=============================================
Configures local credentials and network settings for SmartPong without
exposing private information to Git.

All generated/updated files are verified against .gitignore.
"""

import os
import sys
import socket
import getpass
import shutil
import re
import subprocess
import argparse

REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
FIRMWARE_DIR = os.path.join(REPO_ROOT, "firmware")
MAIN_DIR = os.path.join(FIRMWARE_DIR, "main")
FRONTEND_DIR = os.path.join(REPO_ROOT, "frontend")

SDKCONFIG_PATH = os.path.join(FIRMWARE_DIR, "sdkconfig")
SDKCONFIG_EXAMPLE = os.path.join(FIRMWARE_DIR, "sdkconfig-example")
NETWORK_CONFIG_H = os.path.join(MAIN_DIR, "network_config.h")
FRONTEND_ENV_LOCAL = os.path.join(FRONTEND_DIR, ".env.local")

# Terminal colors
CYAN = "\033[96m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
BOLD = "\033[1m"
RESET = "\033[0m"


def print_banner():
    print(f"{CYAN}{BOLD}")
    print("==================================================================")
    print("                 SmartPong Local Setup & Config                   ")
    print("==================================================================")
    print(f"{RESET}")
    print("Configure your local Wi-Fi, MQTT broker host, and private settings.")
    print("All configured files are strictly ignored by Git to keep your data private.\n")


def get_detected_ip():
    """Attempt to detect the primary local LAN IP address."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = "192.168.1.100"
    finally:
        s.close()
    return ip


def get_detected_hostname():
    """Get the local machine's hostname (without domain suffix)."""
    h = socket.gethostname()
    if h.endswith(".local"):
        h = h[:-6]
    return h


def read_existing_sdkconfig():
    """Extract existing Wi-Fi SSID and password from firmware/sdkconfig if present."""
    ssid = ""
    password = ""
    if os.path.isfile(SDKCONFIG_PATH):
        with open(SDKCONFIG_PATH, "r", encoding="utf-8", errors="ignore") as f:
            for line in f:
                m_ssid = re.match(r'^CONFIG_EXAMPLE_WIFI_SSID="([^"]*)"', line.strip())
                if m_ssid:
                    ssid = m_ssid.group(1)
                m_pw = re.match(r'^CONFIG_EXAMPLE_WIFI_PASSWORD="([^"]*)"', line.strip())
                if m_pw:
                    password = m_pw.group(1)
    return ssid, password


def read_existing_network_config():
    """Extract existing broker host and fallback URI from network_config.h if present."""
    host = ""
    uri = ""
    if os.path.isfile(NETWORK_CONFIG_H):
        with open(NETWORK_CONFIG_H, "r", encoding="utf-8", errors="ignore") as f:
            for line in f:
                m_h = re.match(r'^#define\s+MDNS_BROKER_HOST\s+"([^"]*)"', line.strip())
                if m_h:
                    host = m_h.group(1)
                m_u = re.match(r'^#define\s+MQTT_FALLBACK_URI\s+"([^"]*)"', line.strip())
                if m_u:
                    uri = m_u.group(1)
    return host, uri


def prompt_user(prompt_text, default_val=None, is_secret=False):
    """Prompt the user for input with an optional default value."""
    if default_val:
        display_default = "******" if (is_secret and default_val) else default_val
        full_prompt = f"{BOLD}{prompt_text}{RESET} [{CYAN}{display_default}{RESET}]: "
    else:
        full_prompt = f"{BOLD}{prompt_text}{RESET}: "

    val = input(full_prompt).strip()
    if not val:
        return default_val if default_val is not None else ""
    return val


def update_sdkconfig(ssid, password):
    """Write or update Wi-Fi credentials in firmware/sdkconfig."""
    if not os.path.exists(SDKCONFIG_PATH):
        if os.path.exists(SDKCONFIG_EXAMPLE):
            shutil.copyfile(SDKCONFIG_EXAMPLE, SDKCONFIG_PATH)
        else:
            # Create a minimal fallback
            with open(SDKCONFIG_PATH, "w", encoding="utf-8") as f:
                f.write("# Automatically generated SmartPong sdkconfig\n")

    with open(SDKCONFIG_PATH, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()

    found_ssid = False
    found_pw = False
    new_lines = []

    for line in lines:
        if line.startswith("CONFIG_EXAMPLE_WIFI_SSID="):
            new_lines.append(f'CONFIG_EXAMPLE_WIFI_SSID="{ssid}"\n')
            found_ssid = True
        elif line.startswith("CONFIG_EXAMPLE_WIFI_PASSWORD="):
            new_lines.append(f'CONFIG_EXAMPLE_WIFI_PASSWORD="{password}"\n')
            found_pw = True
        else:
            new_lines.append(line)

    if not found_ssid:
        new_lines.append(f'CONFIG_EXAMPLE_WIFI_SSID="{ssid}"\n')
    if not found_pw:
        new_lines.append(f'CONFIG_EXAMPLE_WIFI_PASSWORD="{password}"\n')

    with open(SDKCONFIG_PATH, "w", encoding="utf-8") as f:
        f.writelines(new_lines)


def write_network_config_h(broker_host, fallback_uri, resolve_timeout_ms=4000):
    """Write network_config.h with local broker host and fallback URI."""
    content = f"""#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

// Auto-generated by configure.py - DO NOT COMMIT TO GIT
// Local broker & mDNS configuration
#define MDNS_BROKER_HOST  "{broker_host}"
#define MQTT_FALLBACK_URI "{fallback_uri}"
#define MDNS_RESOLVE_TIMEOUT_MS {resolve_timeout_ms}

#endif // NETWORK_CONFIG_H
"""
    with open(NETWORK_CONFIG_H, "w", encoding="utf-8") as f:
        f.write(content)


def write_frontend_env(api_base, mqtt_ws_url):
    """Write frontend/.env.local for local network dashboard access."""
    content = f"""# Auto-generated by configure.py - DO NOT COMMIT TO GIT
VITE_API_BASE={api_base}
VITE_MQTT_URL={mqtt_ws_url}
"""
    with open(FRONTEND_ENV_LOCAL, "w", encoding="utf-8") as f:
        f.write(content)


def check_git_ignored(file_path):
    """Check if the given path is ignored by Git."""
    rel_path = os.path.relpath(file_path, REPO_ROOT)
    try:
        res = subprocess.run(
            ["git", "check-ignore", rel_path],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
        )
        return res.returncode == 0
    except Exception:
        return False


def verify_security():
    """Verify and print status of private files."""
    print(f"\n{BOLD}Security & Git Privacy Check:{RESET}")
    files_to_check = [
        (SDKCONFIG_PATH, "Firmware Wi-Fi credentials"),
        (NETWORK_CONFIG_H, "Firmware mDNS / Broker config"),
        (FRONTEND_ENV_LOCAL, "Frontend LAN environment overrides"),
    ]

    all_secure = True
    for fpath, desc in files_to_check:
        rel = os.path.relpath(fpath, REPO_ROOT)
        if os.path.exists(fpath):
            is_ignored = check_git_ignored(fpath)
            if is_ignored:
                print(f"  {GREEN}[✓] {rel:<35} -> Ignored by Git ({desc}){RESET}")
            else:
                print(f"  {RED}[✗] {rel:<35} -> WARNING: NOT IGNORED BY GIT!{RESET}")
                all_secure = False
        else:
            print(f"  {YELLOW}[-] {rel:<35} -> Not present (optional: {desc}){RESET}")

    if all_secure:
        print(f"\n{GREEN}{BOLD}All private configuration files are safely ignored by Git!{RESET}")
    else:
        print(f"\n{RED}{BOLD}Please check your .gitignore before committing!{RESET}")


def show_status():
    """Display currently configured private settings."""
    print_banner()
    print(f"{BOLD}Current Configuration Status:{RESET}\n")

    ssid, pw = read_existing_sdkconfig()
    host, uri = read_existing_network_config()

    masked_pw = ("*" * len(pw)) if pw else "(none)"
    print(f"  Wi-Fi SSID:               {CYAN}{ssid or '(not configured)'}{RESET}")
    print(f"  Wi-Fi Password:           {CYAN}{masked_pw}{RESET}")
    print(f"  mDNS Broker Host:         {CYAN}{host or '(not configured)'}{RESET}")
    print(f"  MQTT Fallback URI:        {CYAN}{uri or '(not configured)'}{RESET}")

    if os.path.exists(FRONTEND_ENV_LOCAL):
        print(f"  Frontend .env.local:      {GREEN}Present{RESET}")
    else:
        print(f"  Frontend .env.local:      {YELLOW}Not created (using default localhost){RESET}")

    verify_security()


def main():
    parser = argparse.ArgumentParser(description="SmartPong Local Setup & Configuration Utility")
    parser.add_argument("--status", action="store_true", help="Display current configuration and privacy status")
    parser.add_argument("--non-interactive", action="store_true", help="Run without prompts using flags or defaults")
    parser.add_argument("--ssid", type=str, default=None, help="Wi-Fi SSID")
    parser.add_argument("--password", type=str, default=None, help="Wi-Fi Password")
    parser.add_argument("--broker-host", type=str, default=None, help="MQTT Broker mDNS hostname (without .local)")
    parser.add_argument("--broker-ip", type=str, default=None, help="MQTT Broker LAN IP address")
    parser.add_argument("--enable-lan-frontend", action="store_true", help="Configure frontend for LAN device access")

    args = parser.parse_args()

    if args.status:
        show_status()
        return

    print_banner()

    detected_ip = get_detected_ip()
    detected_host = get_detected_hostname()

    curr_ssid, curr_pw = read_existing_sdkconfig()
    curr_host, curr_uri = read_existing_network_config()

    default_ssid = args.ssid or curr_ssid or "MyHomeWiFi"
    default_pw = args.password if args.password is not None else curr_pw
    default_host = args.broker_host or curr_host or detected_host
    default_ip = args.broker_ip or (curr_uri.replace("mqtt://", "") if curr_uri else detected_ip)

    if args.non_interactive:
        ssid = default_ssid
        password = default_pw
        broker_host = default_host
        broker_ip = default_ip
        setup_frontend = args.enable_lan_frontend
    else:
        # Step 1: Wi-Fi
        print(f"{YELLOW}{BOLD}Step 1: Wi-Fi Settings for M5StickC Plus ESP32{RESET}")
        ssid = prompt_user("Wi-Fi SSID", default_ssid)
        password = prompt_user("Wi-Fi Password", default_pw, is_secret=True)
        print()

        # Step 2: MQTT Broker Network Settings
        print(f"{YELLOW}{BOLD}Step 2: MQTT Broker & Network Settings{RESET}")
        print(f"Detected Local LAN IP:  {CYAN}{detected_ip}{RESET}")
        print(f"Detected Hostname:      {CYAN}{detected_host}{RESET}\n")

        broker_host = prompt_user("mDNS Hostname for Broker (without .local)", default_host)
        broker_ip = prompt_user("Fallback Broker IP Address", default_ip)
        print()

        # Step 3: Frontend / Remote access
        print(f"{YELLOW}{BOLD}Step 3: Web Dashboard Access (Optional){RESET}")
        lan_choice = prompt_user(
            "Configure web dashboard for access from other devices (e.g., phones on LAN)? (y/N)",
            "N"
        ).lower()
        setup_frontend = lan_choice.startswith("y")
        print()

    fallback_uri = f"mqtt://{broker_ip}" if not broker_ip.startswith("mqtt://") else broker_ip

    # Apply configuration
    print(f"{BOLD}Applying configuration...{RESET}")

    # 1. Update sdkconfig
    update_sdkconfig(ssid, password)
    print(f"  {GREEN}[✓] Updated firmware/sdkconfig with Wi-Fi credentials.{RESET}")

    # 2. Update network_config.h
    write_network_config_h(broker_host, fallback_uri)
    print(f"  {GREEN}[✓] Generated firmware/main/network_config.h with broker settings.{RESET}")

    # 3. Optional frontend env
    if setup_frontend:
        api_base = f"http://{broker_ip}:8000"
        mqtt_ws = f"ws://{broker_ip}:9101"
        write_frontend_env(api_base, mqtt_ws)
        print(f"  {GREEN}[✓] Generated frontend/.env.local for LAN access ({api_base}).{RESET}")
    else:
        if os.path.exists(FRONTEND_ENV_LOCAL):
            os.remove(FRONTEND_ENV_LOCAL)
            print(f"  {YELLOW}[-] Removed frontend/.env.local (defaulting to localhost).{RESET}")

    # Security verification
    verify_security()

    print(f"\n{BOLD}Next Steps:{RESET}")
    print(f"  1. Start the backend & services:  {CYAN}docker compose up --build{RESET}")
    print(f"  2. Build & flash the ESP32:       {CYAN}cd firmware && idf.py build flash monitor{RESET}\n")


if __name__ == "__main__":
    main()
