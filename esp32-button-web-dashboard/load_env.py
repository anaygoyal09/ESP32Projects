Import("env")
import os

env_file = os.path.join(env.get("PROJECT_DIR"), ".env")
if os.path.exists(env_file):
    with open(env_file, "r") as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#") and "=" in line:
                key, val = line.split("=", 1)
                val = val.strip().strip("'\"")
                env.Append(CPPDEFINES=[(key, f'\\"{val}\\"')])
