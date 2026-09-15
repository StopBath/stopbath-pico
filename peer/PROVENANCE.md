# Provenance: the development peer

Copied from the `stopbath-flipper` repository at commit
`3677e61904d854504ac58c5dce6d6e6419836cfc` on 2026-09-15, MIT, unchanged.
Flipper 2.9 requires a development peer in the repository that is developed
against it, so this repository can be worked with no appliance present; the
two copies speak one frozen protocol and must not drift, which is why the
source commit is recorded rather than the peer being referenced across
repositories. It is a test double and nothing more: it interprets no button
and is never the appliance (extension 1.4).

| File | Copied from | sha256 |
|---|---|---|
| `development_peer_core.c` | `peer/development_peer_core.c` | `d00929f9eebcdc38929c4b537c32fc59bc1704a46ad4a061f3934620017cad2e` |
| `development_peer_core.h` | `peer/development_peer_core.h` | `ab0f6ba7dd8bd4de79424927f3d590b8cc02b79ca7551b0a6168f9fb494901ba` |
| `development_peer_shell.c` | `peer/development_peer_shell.c` | `f8dcb5c7a22a69200898ce761770b1642e1ec3ed6bd1365b0edd54fb2af298c4` |
| `../tests/test_development_peer.c` | `tests/test_development_peer.c` | `1680ef97def6e0f689992094e84269a03124a8f546473e06e76a48bd6d8cce1c` |

The shell is POSIX (termios) and builds on Linux and WSL, not under MinGW; it
is a host tool and never part of the firmware.
