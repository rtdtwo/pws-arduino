#!/usr/bin/env python3
"""
Replace <<KEY>> placeholders in the Arduino sketch with values from a .env file,
creating a new sketch file instead of modifying the original.

Usage:
    python apply_env_to_sketch.py

The script expects the .env file at `python/.env` relative to the project root and
writes the processed sketch to `sketch/processed_sketch.ino`.
"""

import re
from pathlib import Path

def load_env(env_path: Path) -> dict:
    """Parse a simple ``KEY=VALUE`` .env file.

    Ignores blank lines and lines starting with ``#``. Strips whitespace around keys and values.
    """
    env = {}
    with env_path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if "=" not in line:
                continue
            key, value = line.split("=", 1)
            env[key.strip()] = value.strip()
    return env

def replace_placeholders(sketch_path: Path, env: dict) -> str:
    """Return sketch content with ``<<KEY>>`` tokens replaced by values from *env*.

    Unmatched placeholders remain unchanged.
    """
    content = sketch_path.read_text(encoding="utf-8")
    return re.sub(r"<<([^>]+)>>", lambda m: env.get(m.group(1), m.group(0)), content)

def main():
    script_dir = Path(__file__).resolve().parent
    env_path = script_dir / ".env"
    original_sketch_path = script_dir.parent / "sketch" / "sketch.ino"
    new_sketch_path = script_dir.parent / "sketch" / "processed_sketch.ino"

    if not env_path.is_file():
        raise FileNotFoundError(f"Env file not found: {env_path}")
    if not original_sketch_path.is_file():
        raise FileNotFoundError(f"Original sketch file not found: {original_sketch_path}")

    env = load_env(env_path)
    new_content = replace_placeholders(original_sketch_path, env)
    new_sketch_path.write_text(new_content, encoding="utf-8")
    print(f"Processed sketch written to '{new_sketch_path}'.")

if __name__ == "__main__":
    main()
