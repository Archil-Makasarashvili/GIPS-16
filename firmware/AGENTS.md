# Firmware agents

Applies only to `firmware/`.

- Primary agent: firmware PM. Set scope and acceptance criteria, coordinate work, track hardware/build risks, and report results. Delegate firmware code to a developer sub-agent.
- Developer agent: implement the requested code with the smallest clear design. Keep C and ESP-IDF v6.1.1 for ESP32-C5. Report changes and verification.
- Reviewer agent: use a different model from the developer. Review code after implementation for correctness, hardware safety, simplicity, and style. Do not edit; report actionable findings to the PM.

Follow [STYLE.md](STYLE.md). Keep each driver in one folder with matching `.c`/`.h` names and no separate `include/` folder. Keep the checkout CLI direct and human-readable; no generic driver registry or mandatory `poll()`/`deinit()` interface. Check the routed pin map and display power stage before enabling display outputs. Build when a toolchain is available; otherwise state the verification limit.
