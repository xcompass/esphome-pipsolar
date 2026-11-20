#pragma once

// PI18 frames captured from an MPP Solar LV2424 (24 V, 3000 VA) on 2026-11-20.
//
// A read-only probe on the ESP32 (UART, 2400 baud) sent each query 3 times, 5 s apart, and logged
// the raw replies. Every CRC checked out as CRC-16/XMODEM with the 0x28/0x0D/0x0A increment rule.
// The 3 replies to each query were identical, except for the clock, which moves with time.
//
// Frame strings represent read_buffer_ content after CRC verification: leading '^' preserved,
// CRC and CR left out. The CRC bytes are given beside each frame.
//
// Not captured:
// - ^P006LST: the inverter answered ^0 (refused) all 3 times, so it has no frame.
// - ^P007PIRI: the boot-time reply was missed because the log stream connected after the boot round,
//   so there is no LV2424_P007PIRI frame.

namespace esphome::pi18::testing {

// ── ^P004T ────────────────────────────────────────────────────────────────────
//
// First of 3 replies (CRC 12 9B). The others were 13:06:23 (F6 A3) and 13:06:28 (47 C8).
// The inverter clock read 13:06:18 while the ESP32's clock read 13:36:50: about 30 minutes behind.
static const char LV2424_P004T[] = "^D01720261120130618";

// ── ^P007ACCT / ^P007ACLT ─────────────────────────────────────────────────────
//
// AC charge window and AC output window, hhmm,hhmm. Both read 0000,0000 (CRC C2 1C for each).
static const char LV2424_P007ACCT[] = "^D0120000,0000";
static const char LV2424_P007ACLT[] = "^D0120000,0000";

// ── ^P009MCHGCR / ^P010MUCHGCR ────────────────────────────────────────────────
//
// Allowed max charging currents: 10 A to 140 A in steps of 10 (CRC 21 6A)
static const char LV2424_P009MCHGCR[] = "^D058010,020,030,040,050,060,070,080,090,100,110,120,130,140";

// Allowed max AC charging currents: 2 A, then 10 A to 60 A in steps of 10 (CRC C8 6A)
static const char LV2424_P010MUCHGCR[] = "^D030002,010,020,030,040,050,060";

}  // namespace esphome::pi18::testing
