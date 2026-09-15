#!/usr/bin/env python3
"""
Holds this repository's copy of protocol.json equal to the StopBath
repository's frozen definition (Pico spec 2.8: the copy and a test that holds
it equal to the appliance's).

The appliance repository is not available to continuous integration, so the
comparison is by digest: the sha256 of docs/peripheral/protocol.json in the
StopBath repository as read on the date below. A change to the frozen
definition is a new protocol version with migration notes (extension 4.1),
at which point this digest is updated deliberately alongside the tables.

Usage:
    python3 scripts/check_protocol_definition.py    # exit 1 if the copy differs
"""

import hashlib
import sys
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
DEFINITION = REPOSITORY_ROOT / "protocol.json"

# StopBath repository, docs/peripheral/protocol.json, read 2026-09-15, frozen
# at FD20 on 2026-09-12.
APPLIANCE_DEFINITION_SHA256 = "5793e16a0b97cd0c122f53c15a1a76b2272a595df0788d0edf065c02b9009d4a"


def main() -> int:
    digest = hashlib.sha256(DEFINITION.read_bytes()).hexdigest()
    if digest != APPLIANCE_DEFINITION_SHA256:
        print(f"protocol.json digest {digest} is not the appliance's frozen {APPLIANCE_DEFINITION_SHA256}")
        return 1
    print("protocol.json matches the appliance's frozen definition")
    return 0


if __name__ == "__main__":
    sys.exit(main())
