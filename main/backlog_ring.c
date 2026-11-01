#include <string.h>
#include "backlog_ring.h"

#define MAGIC        0x4b4c4244u // "DBLK" on flash
#define SLOT_SENT_AT BACKLOG_PAYLOAD_SIZE
#define SLOT_CRC_AT  (BACKLOG_SLOT_SIZE - 2)

_Static_assert(BACKLOG_PAYLOAD_SIZE + 4 == BACKLOG_SLOT_SIZE, "payload + sent + spare + CRC");

typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t seq_inverted; // a header cut short by a power loss fails this
    uint32_t spare;
} sector_header_t;

_Static_assert(sizeof(sector_header_t) == BACKLOG_HEADER_SIZE, "header size");

typedef enum { SLOT_BLANK, SLOT_BAD, SLOT_SENT, SLOT_UNSENT } slot_state_t;

static uint16_t crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xffff;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)(data[i] << 8);
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static uint32_t sector_addr(uint32_t sector)
{
    return sector * BACKLOG_SECTOR_SIZE;
}

static uint32_t slot_addr(backlog_pos_t pos)
{
    return sector_addr(pos.sector) + BACKLOG_HEADER_SIZE + pos.slot * BACKLOG_SLOT_SIZE;
}

static bool is_blank(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (data[i] != 0xff) return false;
    }
    return true;
}

static slot_state_t slot_state(const uint8_t *slot)
{
    if (is_blank(slot, BACKLOG_SLOT_SIZE)) return SLOT_BLANK;
    uint16_t stored = (uint16_t)(slot[SLOT_CRC_AT] | (slot[SLOT_CRC_AT + 1] << 8));
    if (stored != crc16(slot, BACKLOG_PAYLOAD_SIZE)) return SLOT_BAD;
    return slot[SLOT_SENT_AT] == 0xff ? SLOT_UNSENT : SLOT_SENT;
}

static bool read_header(const backlog_ring_t *ring, uint32_t sector, uint32_t *seq)
{
    sector_header_t h;
    if (ring->flash.read(ring->flash.ctx, sector_addr(sector), &h, sizeof(h)) != 0) return false;
    if (h.magic != MAGIC || h.seq != ~h.seq_inverted) return false;
    *seq = h.seq;
    return true;
}

// Moves pos onto the next unsent record, filling slot with it. False at the
// head: nothing left.
static bool next_unsent(const backlog_ring_t *ring, backlog_pos_t *pos, uint8_t *slot)
{
    if (!ring->started) return false;
    for (;;) {
        if (pos->sector == ring->head.sector && pos->slot >= ring->head.slot) return false;
        if (pos->slot >= BACKLOG_SLOTS_PER_SECTOR) {
            pos->sector = (pos->sector + 1) % ring->sectors;
            pos->slot = 0;
            continue;
        }
        if (ring->flash.read(ring->flash.ctx, slot_addr(*pos), slot, BACKLOG_SLOT_SIZE) != 0) return false;
        if (slot_state(slot) == SLOT_UNSENT) return true;
        pos->slot++;
    }
}

int backlog_ring_open(backlog_ring_t *ring, const backlog_flash_t *flash, uint32_t sectors)
{
    if (sectors < 2) return -1;
    *ring = (backlog_ring_t){.flash = *flash, .sectors = sectors};

    for (uint32_t s = 0; s < sectors; s++) {
        uint32_t seq;
        if (read_header(ring, s, &seq) && (!ring->started || seq > ring->seq)) {
            ring->started = true;
            ring->seq = seq;
            ring->head.sector = s;
        }
    }
    if (!ring->started) return 0;

    // Head slot: just past the last slot in use, even a damaged one, since
    // it can't be written again without an erase.
    ring->head.slot = 0;
    for (uint32_t i = BACKLOG_SLOTS_PER_SECTOR; i > 0; i--) {
        uint8_t slot[BACKLOG_SLOT_SIZE];
        backlog_pos_t pos = {ring->head.sector, i - 1};
        if (ring->flash.read(ring->flash.ctx, slot_addr(pos), slot, sizeof(slot)) != 0) return -1;
        if (!is_blank(slot, sizeof(slot))) {
            ring->head.slot = i;
            break;
        }
    }

    // Oldest sector: the first one with a header, going round from the head.
    ring->tail = ring->head;
    ring->tail.slot = 0;
    for (uint32_t step = 1; step < sectors; step++) {
        uint32_t s = (ring->head.sector + step) % sectors;
        uint32_t seq;
        if (read_header(ring, s, &seq)) {
            ring->tail.sector = s;
            break;
        }
    }
    return 0;
}

// Erases the next sector and makes it the head; a full ring loses its oldest.
static int start_next_sector(backlog_ring_t *ring)
{
    uint32_t next = ring->started ? (ring->head.sector + 1) % ring->sectors : 0;
    if (ring->started && ring->tail.sector == next) {
        ring->tail = (backlog_pos_t){(next + 1) % ring->sectors, 0};
    }
    if (ring->flash.erase_sector(ring->flash.ctx, sector_addr(next)) != 0) return -1;
    uint32_t seq = ring->started ? ring->seq + 1 : 1;
    sector_header_t h = {.magic = MAGIC, .seq = seq, .seq_inverted = ~seq, .spare = 0xffffffffu};
    if (ring->flash.write(ring->flash.ctx, sector_addr(next), &h, sizeof(h)) != 0) return -1;
    if (!ring->started) ring->tail = (backlog_pos_t){next, 0};
    ring->started = true;
    ring->seq = seq;
    ring->head = (backlog_pos_t){next, 0};
    return 0;
}

int backlog_ring_append(backlog_ring_t *ring, const void *payload)
{
    if (!ring->started || ring->head.slot >= BACKLOG_SLOTS_PER_SECTOR) {
        if (start_next_sector(ring) != 0) return -1;
    }
    uint8_t slot[BACKLOG_SLOT_SIZE];
    memset(slot, 0xff, sizeof(slot));
    memcpy(slot, payload, BACKLOG_PAYLOAD_SIZE);
    uint16_t crc = crc16(slot, BACKLOG_PAYLOAD_SIZE);
    slot[SLOT_CRC_AT] = (uint8_t)(crc & 0xff);
    slot[SLOT_CRC_AT + 1] = (uint8_t)(crc >> 8);
    backlog_pos_t pos = ring->head;
    ring->head.slot++; // a failed write still used the slot up
    return ring->flash.write(ring->flash.ctx, slot_addr(pos), slot, sizeof(slot));
}

int backlog_ring_read(backlog_ring_t *ring, void *payloads, int max)
{
    uint8_t slot[BACKLOG_SLOT_SIZE];
    uint8_t *out = payloads;
    int count = 0;
    // Moving the tail itself keeps later calls from re-reading what's sent.
    if (max <= 0 || !next_unsent(ring, &ring->tail, slot)) return 0;
    backlog_pos_t pos = ring->tail;
    do {
        memcpy(out + (size_t)count * BACKLOG_PAYLOAD_SIZE, slot, BACKLOG_PAYLOAD_SIZE);
        count++;
        pos.slot++;
    } while (count < max && next_unsent(ring, &pos, slot));
    return count;
}

int backlog_ring_mark_sent(backlog_ring_t *ring, int count)
{
    uint8_t slot[BACKLOG_SLOT_SIZE];
    const uint8_t sent = 0;
    for (int i = 0; i < count; i++) {
        if (!next_unsent(ring, &ring->tail, slot)) return -1;
        uint32_t addr = slot_addr(ring->tail) + SLOT_SENT_AT;
        if (ring->flash.write(ring->flash.ctx, addr, &sent, 1) != 0) return -1;
        ring->tail.slot++;
    }
    return 0;
}
