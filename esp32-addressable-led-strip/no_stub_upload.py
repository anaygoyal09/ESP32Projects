Import("env")

# This ESP32 board reliably enters the ROM bootloader but its temporary
# high-speed flasher stub stops responding when the application image starts.
# Make normal `pio run --target upload` commands use the ROM loader instead.
flags = list(env.get("UPLOADERFLAGS", []))

if "write_flash" in flags and "--no-stub" not in flags:
    flags.insert(flags.index("write_flash"), "--no-stub")

# Compressed transfer requires the flasher stub. ROM-mode writes are
# uncompressed and slower, but they are verified and reliable on this board.
flags = [flag for flag in flags if flag != "-z"]

env.Replace(UPLOADERFLAGS=flags)
