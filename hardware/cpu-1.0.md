# daath: hardware CPU measurement

Machine commit: `ea60d4a7b1b67e4f00ac0ff8641ad28bbe5236c7`. Benchmark: `opxy-synth-c8-sweep-v1`. Scope: whole voice, including oscillator/filter.

Heaviest of the midpoint run, 54 measured settings (each control through its positions with the others at their defaults) and every control at its heaviest position, each the median of 3 captures: **3.461%** of the stock 580,352-cycle block budget. Midpoint run: 3.406%. Every control at its heaviest position: 2.661%. Heaviest single setting: Rot 25.0%.

| Setting | Median |
| --- | ---: |
| midpoint (stock) | 3.406% |
| Swarm 0.0% | 2.681% |
| Swarm 12.5% | 3.438% |
| Swarm 25.0% | 3.439% |
| Swarm 37.5% | 3.441% |
| Swarm 50.0% | 3.435% |
| Swarm 62.5% | 3.426% |
| Swarm 75.0% | 3.450% |
| Swarm 87.5% | 3.426% |
| Swarm 100.0% | 3.435% |
| Grave 0.0% | 3.426% |
| Grave 12.5% | 3.435% |
| Grave 25.0% | 3.427% |
| Grave 37.5% | 3.435% |
| Grave 50.0% | 3.426% |
| Grave 62.5% | 3.435% |
| Grave 75.0% | 3.426% |
| Grave 87.5% | 3.435% |
| Grave 100.0% | 3.426% |
| Hex 0.0% | 3.435% |
| Hex 12.5% | 3.426% |
| Hex 25.0% | 3.435% |
| Hex 37.5% | 3.426% |
| Hex 50.0% | 3.436% |
| Hex 62.5% | 3.427% |
| Hex 75.0% | 3.438% |
| Hex 87.5% | 3.429% |
| Hex 100.0% | 3.436% |
| Murk 0.0% | 3.413% |
| Murk 12.5% | 3.421% |
| Murk 25.0% | 3.415% |
| Murk 37.5% | 3.423% |
| Murk 50.0% | 3.421% |
| Murk 62.5% | 3.430% |
| Murk 75.0% | 3.421% |
| Murk 87.5% | 3.432% |
| Murk 100.0% | 3.424% |
| Rot 0.0% | 3.435% |
| Rot 12.5% | 3.452% |
| Rot 25.0% | 3.461% |
| Rot 37.5% | 3.452% |
| Rot 50.0% | 3.461% |
| Rot 62.5% | 3.453% |
| Rot 75.0% | 3.459% |
| Rot 87.5% | 3.452% |
| Rot 100.0% | 3.461% |
| Wow 0.0% | 3.427% |
| Wow 12.5% | 3.436% |
| Wow 25.0% | 3.426% |
| Wow 37.5% | 3.450% |
| Wow 50.0% | 3.426% |
| Wow 62.5% | 3.435% |
| Wow 75.0% | 3.426% |
| Wow 87.5% | 3.435% |
| Wow 100.0% | 3.426% |
| combined (as read from the reports), differing between runs: Swarm 75.0%, Grave 12.5%, Hex 75.0%, Murk 87.5%, Rot 25.0%, Wow 37.5%; Swarm 75.0%, Grave 87.5%, Hex 75.0%, Murk 87.5%, Rot 25.0%, Wow 37.5%; Swarm 75.0%, Grave 12.5%, Hex 75.0%, Murk 62.5%, Rot 25.0%, Wow 37.5% | 2.661% |

Recommended: at most 3.5%. Within it.

This is a synthetic measurement on the USB-MIDI command endpoint, not total device CPU, maximum polyphony or other combinations of control settings. Graphics are excluded. Callback tests exclude sample preparation, allocation, stock mixing and voice processing; they include stopwatch-call overhead and may include interrupt time. See docs/profiling.md for the versioned workloads.

Firmware SHA-256: `91698801824a9f4322fa0b95424230eecb9b284494b5faf3b0e62327c45384e2`.

Compile report SHA-256: `cf3edeb34fb6aa38f4e8d1547ff260458be41d4d0fe919bc44bde01bd0d1a56e`.

Attribution uses the user-confirmed installed package, expected case order and exact report count; the device does not attest its firmware hash. Raw captures remain private because they contain device identity.
