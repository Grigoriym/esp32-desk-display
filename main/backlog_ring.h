#pragma once

// Ring log of fixed-size records on NOR flash, for readings the dashboard
// server couldn't take. Pure, no ESP-IDF dependencies: flash is reached through
// backlog_flash_t, so the host tests can run it on a fake flash and cut the
// power at any point.
//
// Sector: 16-byte header (magic + sequence number), then slots of 40 bytes.
// Slot: payload, a "sent" byte, one spare byte, CRC-16 of the payload. Sending
// clears the sent byte in place (flash bits can go 1 -> 0 without an erase).
// When the ring is full, the oldest sector is erased and its records are lost.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BACKLOG_SECTOR_SIZE      4096
#define BACKLOG_PAYLOAD_SIZE     36
#define BACKLOG_SLOT_SIZE        40
#define BACKLOG_HEADER_SIZE      16
#define BACKLOG_SLOTS_PER_SECTOR ((BACKLOG_SECTOR_SIZE - BACKLOG_HEADER_SIZE) / BACKLOG_SLOT_SIZE)

// Each returns 0 on success. Addresses are relative to the ring's start.
typedef struct {
    void *ctx;
    int (*read)(void *ctx, uint32_t addr, void *buf, size_t len);
    int (*write)(void *ctx, uint32_t addr, const void *buf, size_t len);
    int (*erase_sector)(void *ctx, uint32_t addr);
} backlog_flash_t;

typedef struct {
    uint32_t sector;
    uint32_t slot;
} backlog_pos_t;

typedef struct {
    backlog_flash_t flash;
    uint32_t sectors;
    bool started;       // false until a first sector has a header
    backlog_pos_t head; // the next slot to write
    backlog_pos_t tail; // no unsent record before this
    uint32_t seq;       // the head sector's sequence number
} backlog_ring_t;

// Finds the newest and oldest records left by the previous boot. Needs at
// least 2 sectors. Returns 0 on success.
int backlog_ring_open(backlog_ring_t *ring, const backlog_flash_t *flash, uint32_t sectors);

// Stores one record of BACKLOG_PAYLOAD_SIZE bytes. Returns 0 on success.
int backlog_ring_append(backlog_ring_t *ring, const void *payload);

// Copies up to max unsent records, oldest first, into payloads (max *
// BACKLOG_PAYLOAD_SIZE bytes) without changing anything on flash. Returns how
// many. Records that fail their CRC (a write cut short) are skipped.
int backlog_ring_read(backlog_ring_t *ring, void *payloads, int max);

// Marks the count oldest unsent records as sent. Returns 0 on success.
int backlog_ring_mark_sent(backlog_ring_t *ring, int count);
