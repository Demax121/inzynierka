# Documentation Index {#documentation-index}

Primary English Docs:
- [Root README](../README.md)
- [Complete Documentation](./COMPLETE_DOCUMENTATION.md)
- [JSON Payload Reference](./json-payloads.md)

Polish Docs:
- [README (PL)](./README_pl.md)
- [Pełna Dokumentacja (PL)](./COMPLETE_DOCUMENTATION_PL.md)
- [Referencja Payload (PL)](./json-payloads_pl.md)

Supplementary:
- [Technical Dossier (PL)](./TECHNICAL_DOKUMENTACJA_PL.md) (comprehensive engineering summary)

Quick Links:
- Simulation Quickstart: see sections in each doc or run:
  ```
  cd jsServer
  bun run testDeviceSim.js room_stats <device_api_key> <16charKey>
  ```
- Encryption: AES-128-GCM envelope `{nonce, payload, tag, alg}`
- Channels: door_sensor, room_stats, lux_sensor, main_lights, air_conditioning

Keep this index updated when adding new documentation artifacts.
