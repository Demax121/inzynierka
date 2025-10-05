# Documentation Changelog {#documentation-changelog}

Chronological evolution of project documentation for thesis appendix.

## Phase 1 – Initial Architecture Notes {#phase-1}
- Basic README describing stack (PostgreSQL, PHP endpoints, Bun WS, Vue frontend, ESP32 devices).
- Minimal mention of AES-128-CBC (pre‑GCM) and device channels.

## Phase 2 – Encryption Migration Planning {#phase-2}
- Added plan outlining move from AES-128-CBC (no integrity) to AES-128-GCM (AEAD): nonce strategy, tag handling, envelope adjustments.
- Expanded security rationale section (threat model, rejected alternatives).

## Phase 3 – Full GCM Migration & Refactor {#phase-3}
- Updated all references to new envelope `{nonce, payload, tag, alg}`.
- Removed legacy IV terminology; clarified 12‑byte nonce (24 hex) + 16‑byte tag (32 hex).
- Added firmware crypto flow description (generate nonce → encrypt → send) and server decrypt sequence.

## Phase 4 – Firmware & Component Commentary Expansion {#phase-4}
- Inlined comprehensive comments in ESP32 sketches (hysteresis logic, debounce, identification handshake).
- Documented each Vue component’s responsibility and associated WebSocket channel.

## Phase 5 – Comprehensive English Documentation Suite {#phase-5}
- Created `COMPLETE_DOCUMENTATION.md` with: ER diagram (ASCII), runtime flows, channel logic, security model, future roadmap.
- Added JSON payload reference (`json-payloads.md`) consolidating shapes for WS + PHP endpoints.

## Phase 6 – Polish Localization Pass {#phase-6}
- Added Polish high-level README (`README_pl.md`).
- Added full Polish deep-dive (`COMPLETE_DOCUMENTATION_PL.md`).
- Added Polish payload reference (`json-payloads_pl.md`).

## Phase 7 – Simulation & Developer Onboarding Enhancements {#phase-7}
- Inserted "Device Simulation Quickstart" sections (EN + PL) in all major docs with `testDeviceSim.js` usage.
- Clarified integrity test procedure (alter hex nibble to observe rejection).

## Phase 8 – Operational & Troubleshooting Content (PL) {#phase-8}
- Added Polish troubleshooting matrix (symptom / cause / resolution).
- Added maintenance checklists (rotation cadence, log review, pruning tasks).

## Phase 9 – Central Index & Technical Dossier {#phase-9}
- Created `INDEX.md` linking every doc artifact for easy navigation.
- Authored `TECHNICAL_DOKUMENTACJA_PL.md` (engineering dossier) aggregating repository structure, endpoints, channel specs, security, and rationale tables.

## Phase 10 – Heading Anchor Normalization {#phase-10}
- Added explicit Markdown IDs to all headings across every documentation file for stable deep-linking (required for thesis citations and internal cross references).

## Phase 11 – Changelog Formalization (Current) {#phase-11}
- Introduced this `DOCS_CHANGELOG.md` to chronicle iterative documentation evolution.

## Pending / Future (Proposed) {#future}
- Add section detailing replay protection implementation once nonce cache introduced.
- Add AAD binding update notes (channel + api_key) if adopted.
- Introduce observability/logging appendix (structured log schema + sample entries).

---
Generated: 2025-10-05. Keep this file updated with each substantive documentation refactor or addition.
