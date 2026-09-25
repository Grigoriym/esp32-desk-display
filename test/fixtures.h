#pragma once

// Reads test/fixtures/<name> into a malloc'd, NUL-terminated string (caller
// frees). Fails the running test if the file can't be read.

#include <stdio.h>
#include <stdlib.h>
#include "unity.h"

#ifndef FIXTURES_DIR
#error "build with -DFIXTURES_DIR=\"...\" (tools/test.sh does)"
#endif

static inline char *fixture_read(const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", FIXTURES_DIR, name);
    FILE *f = fopen(path, "rb");
    if (!f) TEST_FAIL_MESSAGE(path);
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char *buf = malloc(len + 1);
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_EQUAL(len, (long)fread(buf, 1, len, f));
    buf[len] = '\0';
    fclose(f);
    return buf;
}
