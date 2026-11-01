#include <string.h>
#include "unity.h"
#include "backlog_ring.h"

// A fake NOR flash: writes can only clear bits, an erase sets a sector to 0xff.
// write_budget cuts the power: the write that runs out stores only part of its
// bytes, and every later operation fails until the "reboot" (open_ring()).
#define SECTORS  4
#define NO_LIMIT (-1)

static uint8_t flash_mem[SECTORS * BACKLOG_SECTOR_SIZE];
static int write_budget;
static bool power_lost;
static int erase_count;

static int fake_read(void *ctx, uint32_t addr, void *buf, size_t len)
{
    (void)ctx;
    TEST_ASSERT_TRUE(addr + len <= sizeof(flash_mem));
    memcpy(buf, flash_mem + addr, len);
    return 0;
}

static int fake_write(void *ctx, uint32_t addr, const void *buf, size_t len)
{
    (void)ctx;
    TEST_ASSERT_TRUE(addr + len <= sizeof(flash_mem));
    if (power_lost) return -1;
    size_t n = len;
    if (write_budget != NO_LIMIT) {
        if ((size_t)write_budget < len) {
            n = (size_t)write_budget;
            power_lost = true;
        }
        write_budget -= (int)n;
    }
    const uint8_t *src = buf;
    for (size_t i = 0; i < n; i++) flash_mem[addr + i] &= src[i];
    return power_lost ? -1 : 0;
}

static int fake_erase(void *ctx, uint32_t addr)
{
    (void)ctx;
    TEST_ASSERT_EQUAL_UINT32(0, addr % BACKLOG_SECTOR_SIZE);
    if (power_lost) return -1;
    memset(flash_mem + addr, 0xff, BACKLOG_SECTOR_SIZE);
    erase_count++;
    return 0;
}

static const backlog_flash_t FLASH = {.read = fake_read, .write = fake_write, .erase_sector = fake_erase};
static backlog_ring_t ring;

static void open_ring(void)
{
    power_lost = false;
    write_budget = NO_LIMIT;
    TEST_ASSERT_EQUAL_INT(0, backlog_ring_open(&ring, &FLASH, SECTORS));
}

static void append(uint32_t value)
{
    uint8_t payload[BACKLOG_PAYLOAD_SIZE];
    memset(payload, 0x5a, sizeof(payload));
    memcpy(payload, &value, sizeof(value));
    TEST_ASSERT_EQUAL_INT(0, backlog_ring_append(&ring, payload));
}

// Reads up to max values, oldest first; returns how many.
static int read_values(uint32_t *values, int max)
{
    static uint8_t payloads[SECTORS * BACKLOG_SLOTS_PER_SECTOR][BACKLOG_PAYLOAD_SIZE];
    TEST_ASSERT_TRUE(max <= SECTORS * BACKLOG_SLOTS_PER_SECTOR);
    int n = backlog_ring_read(&ring, payloads, max);
    for (int i = 0; i < n; i++) memcpy(&values[i], payloads[i], sizeof(values[i]));
    return n;
}

static void assert_values(uint32_t first, int expected_count)
{
    static uint32_t values[SECTORS * BACKLOG_SLOTS_PER_SECTOR];
    TEST_ASSERT_EQUAL_INT(expected_count, read_values(values, SECTORS * BACKLOG_SLOTS_PER_SECTOR));
    for (int i = 0; i < expected_count; i++) TEST_ASSERT_EQUAL_UINT32(first + (uint32_t)i, values[i]);
}

void setUp(void)
{
    memset(flash_mem, 0xff, sizeof(flash_mem));
    erase_count = 0;
    open_ring();
}

void tearDown(void)
{
}

static void test_empty_ring_has_nothing_and_erases_nothing(void)
{
    assert_values(0, 0);
    TEST_ASSERT_EQUAL_INT(0, erase_count);
}

static void test_records_come_back_oldest_first(void)
{
    for (uint32_t i = 1; i <= 5; i++) append(i);
    assert_values(1, 5);
}

static void test_payload_comes_back_whole(void)
{
    uint8_t in[BACKLOG_PAYLOAD_SIZE], out[BACKLOG_PAYLOAD_SIZE];
    for (int i = 0; i < BACKLOG_PAYLOAD_SIZE; i++) in[i] = (uint8_t)(i + 1);
    TEST_ASSERT_EQUAL_INT(0, backlog_ring_append(&ring, in));
    TEST_ASSERT_EQUAL_INT(1, backlog_ring_read(&ring, out, 1));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(in, out, BACKLOG_PAYLOAD_SIZE);
}

static void test_read_respects_max_and_changes_nothing(void)
{
    for (uint32_t i = 1; i <= 5; i++) append(i);
    uint32_t values[5];
    TEST_ASSERT_EQUAL_INT(2, read_values(values, 2));
    TEST_ASSERT_EQUAL_UINT32(1, values[0]);
    TEST_ASSERT_EQUAL_UINT32(2, values[1]);
    assert_values(1, 5);
}

static void test_mark_sent_moves_on(void)
{
    for (uint32_t i = 1; i <= 5; i++) append(i);
    TEST_ASSERT_EQUAL_INT(0, backlog_ring_mark_sent(&ring, 3));
    assert_values(4, 2);
    TEST_ASSERT_EQUAL_INT(0, backlog_ring_mark_sent(&ring, 2));
    assert_values(0, 0);
    append(6);
    assert_values(6, 1);
}

static void test_mark_sent_past_the_end_fails(void)
{
    append(1);
    TEST_ASSERT_NOT_EQUAL(0, backlog_ring_mark_sent(&ring, 2));
}

static void test_survives_reboot(void)
{
    for (uint32_t i = 1; i <= 5; i++) append(i);
    TEST_ASSERT_EQUAL_INT(0, backlog_ring_mark_sent(&ring, 2));
    open_ring();
    assert_values(3, 3);
    append(6);
    assert_values(3, 4);
}

static void test_crosses_sectors(void)
{
    uint32_t total = BACKLOG_SLOTS_PER_SECTOR * 2 + 3;
    for (uint32_t i = 1; i <= total; i++) append(i);
    assert_values(1, (int)total);
    open_ring();
    assert_values(1, (int)total);
}

static void test_full_sector_reopens_and_continues(void)
{
    for (uint32_t i = 1; i <= BACKLOG_SLOTS_PER_SECTOR; i++) append(i);
    open_ring();
    append(BACKLOG_SLOTS_PER_SECTOR + 1);
    assert_values(1, BACKLOG_SLOTS_PER_SECTOR + 1);
}

// Writing past the capacity costs the oldest sector, never the newest record.
static void test_full_ring_drops_oldest_sector(void)
{
    uint32_t capacity = SECTORS * BACKLOG_SLOTS_PER_SECTOR;
    for (uint32_t i = 1; i <= capacity + 1; i++) append(i);
    uint32_t oldest = BACKLOG_SLOTS_PER_SECTOR + 1;
    assert_values(oldest, (int)(capacity + 1 - BACKLOG_SLOTS_PER_SECTOR));
    open_ring();
    assert_values(oldest, (int)(capacity + 1 - BACKLOG_SLOTS_PER_SECTOR));
}

static void test_wraps_many_times_and_keeps_newest(void)
{
    uint32_t total = SECTORS * BACKLOG_SLOTS_PER_SECTOR * 5 + 7;
    uint32_t last = 0;
    for (uint32_t i = 1; i <= total; i++) {
        append(i);
        last = i;
    }
    static uint32_t values[SECTORS * BACKLOG_SLOTS_PER_SECTOR];
    int n = read_values(values, SECTORS * BACKLOG_SLOTS_PER_SECTOR);
    TEST_ASSERT_GREATER_THAN_INT(SECTORS * BACKLOG_SLOTS_PER_SECTOR - BACKLOG_SLOTS_PER_SECTOR - 1, n);
    TEST_ASSERT_EQUAL_UINT32(last, values[n - 1]);
    for (int i = 1; i < n; i++) TEST_ASSERT_EQUAL_UINT32(values[i - 1] + 1, values[i]);
}

// Everything sent, then the ring goes round: old sent records never come back.
static void test_sent_records_stay_sent_across_wrap(void)
{
    for (uint32_t i = 1; i <= BACKLOG_SLOTS_PER_SECTOR * 3; i++) append(i);
    TEST_ASSERT_EQUAL_INT(0, backlog_ring_mark_sent(&ring, BACKLOG_SLOTS_PER_SECTOR * 3));
    for (uint32_t i = 1000; i < 1000 + BACKLOG_SLOTS_PER_SECTOR * 2; i++) append(i);
    assert_values(1000, BACKLOG_SLOTS_PER_SECTOR * 2);
    open_ring();
    assert_values(1000, BACKLOG_SLOTS_PER_SECTOR * 2);
}

// Power lost at every byte of a record's write: earlier records stay, the torn
// one never comes back as data, and logging carries on after the reboot.
static void test_power_loss_during_record_write(void)
{
    for (int cut = 0; cut < BACKLOG_SLOT_SIZE; cut++) {
        setUp();
        append(1);
        write_budget = cut;
        uint8_t payload[BACKLOG_PAYLOAD_SIZE];
        memset(payload, 0x11, sizeof(payload));
        TEST_ASSERT_NOT_EQUAL(0, backlog_ring_append(&ring, payload));
        open_ring();
        append(2);
        uint32_t values[3];
        TEST_ASSERT_EQUAL_INT(2, read_values(values, 3));
        TEST_ASSERT_EQUAL_UINT32(1, values[0]);
        TEST_ASSERT_EQUAL_UINT32(2, values[1]);
    }
}

// Power lost writing a new sector's header: the records before it survive, and
// the half-started sector is erased again on the next try.
static void test_power_loss_during_sector_header(void)
{
    for (int cut = 0; cut < BACKLOG_HEADER_SIZE; cut++) {
        setUp();
        for (uint32_t i = 1; i <= BACKLOG_SLOTS_PER_SECTOR; i++) append(i);
        write_budget = cut;
        uint8_t payload[BACKLOG_PAYLOAD_SIZE] = {0};
        TEST_ASSERT_NOT_EQUAL(0, backlog_ring_append(&ring, payload));
        open_ring();
        assert_values(1, BACKLOG_SLOTS_PER_SECTOR);
        append(BACKLOG_SLOTS_PER_SECTOR + 1);
        assert_values(1, BACKLOG_SLOTS_PER_SECTOR + 1);
    }
}

// Power lost between sending and marking: the record is sent again, which the
// server treats as the same point (same timestamp). Nothing is lost.
static void test_power_loss_before_mark_keeps_record(void)
{
    append(1);
    write_budget = 0;
    TEST_ASSERT_NOT_EQUAL(0, backlog_ring_mark_sent(&ring, 1));
    open_ring();
    assert_values(1, 1);
}

// Old contents in the partition (or plain garbage) don't look like records.
static void test_garbage_flash_is_treated_as_empty(void)
{
    memset(flash_mem, 0xa5, sizeof(flash_mem));
    open_ring();
    assert_values(0, 0);
    append(1);
    assert_values(1, 1);
}

static void test_damaged_record_is_skipped(void)
{
    for (uint32_t i = 1; i <= 3; i++) append(i);
    flash_mem[BACKLOG_HEADER_SIZE + BACKLOG_SLOT_SIZE + 4] ^= 0x01; // inside record 2
    open_ring();
    uint32_t values[3];
    TEST_ASSERT_EQUAL_INT(2, read_values(values, 3));
    TEST_ASSERT_EQUAL_UINT32(1, values[0]);
    TEST_ASSERT_EQUAL_UINT32(3, values[1]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_empty_ring_has_nothing_and_erases_nothing);
    RUN_TEST(test_records_come_back_oldest_first);
    RUN_TEST(test_payload_comes_back_whole);
    RUN_TEST(test_read_respects_max_and_changes_nothing);
    RUN_TEST(test_mark_sent_moves_on);
    RUN_TEST(test_mark_sent_past_the_end_fails);
    RUN_TEST(test_survives_reboot);
    RUN_TEST(test_crosses_sectors);
    RUN_TEST(test_full_sector_reopens_and_continues);
    RUN_TEST(test_full_ring_drops_oldest_sector);
    RUN_TEST(test_wraps_many_times_and_keeps_newest);
    RUN_TEST(test_sent_records_stay_sent_across_wrap);
    RUN_TEST(test_power_loss_during_record_write);
    RUN_TEST(test_power_loss_during_sector_header);
    RUN_TEST(test_power_loss_before_mark_keeps_record);
    RUN_TEST(test_garbage_flash_is_treated_as_empty);
    RUN_TEST(test_damaged_record_is_skipped);
    return UNITY_END();
}
