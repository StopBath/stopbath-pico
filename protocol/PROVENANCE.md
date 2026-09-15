# Provenance: the protocol library

Copied from the `stopbath-flipper` repository at commit
`3677e61904d854504ac58c5dce6d6e6419836cfc` on 2026-09-15, MIT. The library is
the peripheral's side of the contract the StopBath repository owns
(`docs/peripheral/PROTOCOL.md` there), generated from and tested against the
same table, so a second peripheral reusing it is exactly what Flipper 1.4
intends. The Flipper repository's `PROTOCOL.md` change record is the argument
for its shape.

| File | Copied from | sha256 of the source at that commit |
|---|---|---|
| `remote_protocol.c` | `protocol/remote_protocol.c` | `c11d3214f83da2527514253b7929a3009ee0514cf8c33bae779ea97bb68ff5f8` |
| `remote_protocol.h` | `protocol/remote_protocol.h` | `020d58d1e86ddae7f162c89c51ffda118a842efaa569fd41b6f9d477d08f3518` |
| `../scripts/generate_protocol_tables.py` | `scripts/generate_protocol_tables.py` | `51b0f6df936ead403f8d4eacd114607db805fb73e304bf2077b7f181e4951cbb` |
| `../fuzz/fuzz_remote_protocol.c` | `fuzz/fuzz_remote_protocol.c` | `a8af169e7f8f3f1facd98c3eee148230a183f7d242da206e4eadd81096d30426` |
| `../tests/test_remote_protocol.c` | `tests/test_remote_protocol.c` | `57a1711f1f8bc48ff980eb8b7f37ef81cfa47f88f5dbadc81d566e5a9e027f3b` |

`remote_protocol_tables.h` and `remote_protocol_tables.c` are not copied:
they are generated here by the script from this repository's `protocol.json`,
and `scripts/generate_protocol_tables.py --check` fails if they and the table
ever differ.

## The definition

`../protocol.json` is the StopBath repository's `docs/peripheral/protocol.json`
(sha256 `5793e16a0b97cd0c122f53c15a1a76b2272a595df0788d0edf065c02b9009d4a`),
frozen at `FD20` on 2026-09-12, not the Flipper repository's copy. The two
differ by the status line and by one bound the appliance added at promotion,
`maximum_wifi_payload_length_for_qr` (53), which the generator emits as one
more `#define`; the tables here therefore differ from the Flipper's generated
tables by exactly that line. `scripts/check_protocol_definition.py` holds the
copy to the appliance's digest.

## Verifying

```bash
python3 scripts/check_protocol_definition.py
```

```bash
python3 scripts/generate_protocol_tables.py --check
```
