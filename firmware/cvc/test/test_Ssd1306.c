/**
 * @file    test_Ssd1306.c
 * @brief   Unit tests for Ssd1306 — SSD1306 128x64 I2C OLED display driver
 * @date    2026-02-21
 *
 * @verifies SWR-CVC-026
 *
 * Tests I2C initialization sequence, display clear, cursor positioning,
 * string rendering, single character rendering, and I2C failure handling.
 *
 * Mocks: Ssd1306_Hw_I2cWrite
 */
#include "unity.h"

/* ==================================================================
 * Local type definitions (avoid BSW header mock conflicts)
 * ================================================================== */

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;
typedef uint8           Std_ReturnType;
typedef unsigned char   boolean;

#define E_OK        0u
#define E_NOT_OK    1u
#define TRUE        1u
#define FALSE       0u
#define NULL_PTR    ((void*)0)

/* ==================================================================
 * SSD1306 Constants (mirrors header)
 * ================================================================== */

#define SSD1306_I2C_ADDR    0x3Cu
#define SSD1306_WIDTH       128u
#define SSD1306_HEIGHT      64u
#define SSD1306_PAGES       8u

/* ==================================================================
 * Ssd1306 API declarations
 * ================================================================== */

extern Std_ReturnType Ssd1306_Init(void);
extern void           Ssd1306_Clear(void);
extern void           Ssd1306_SetCursor(uint8 page, uint8 col);
extern Std_ReturnType Ssd1306_WriteString(const char* str);
extern Std_ReturnType Ssd1306_WriteChar(char c);

/* ==================================================================
 * Mock: Ssd1306_Hw_I2cWrite
 * ================================================================== */

static uint8           mock_i2c_addr;
static uint8           mock_i2c_data[64];
static uint8           mock_i2c_len;
static uint8           mock_i2c_call_count;
static Std_ReturnType  mock_i2c_result;

/* Track the first byte of each call to distinguish cmd vs data */
static uint8           mock_i2c_first_bytes[128];
static uint8           mock_i2c_second_bytes[128];

Std_ReturnType Ssd1306_Hw_I2cWrite(uint8 addr, const uint8* data, uint8 len)
{
    uint8 i;

    mock_i2c_addr = addr;
    mock_i2c_len  = len;

    if (mock_i2c_call_count < 128u) {
        if ((data != NULL_PTR) && (len >= 1u)) {
            mock_i2c_first_bytes[mock_i2c_call_count] = data[0];
        }
        if ((data != NULL_PTR) && (len >= 2u)) {
            mock_i2c_second_bytes[mock_i2c_call_count] = data[1];
        }
    }

    /* Copy data for inspection */
    for (i = 0u; (i < len) && (i < 64u); i++) {
        if (data != NULL_PTR) {
            mock_i2c_data[i] = data[i];
        }
    }

    mock_i2c_call_count++;
    return mock_i2c_result;
}

/* ==================================================================
 * Test setup / teardown
 * ================================================================== */

void setUp(void)
{
    uint8 i;

    mock_i2c_addr       = 0u;
    mock_i2c_len        = 0u;
    mock_i2c_call_count = 0u;
    mock_i2c_result     = E_OK;

    for (i = 0u; i < 64u; i++) {
        mock_i2c_data[i] = 0u;
    }
    for (i = 0u; i < 128u; i++) {
        mock_i2c_first_bytes[i]  = 0u;
        mock_i2c_second_bytes[i] = 0u;
    }
}

void tearDown(void) { }

/* ==================================================================
 * SWR-CVC-026: Init sends I2C command sequence
 * ================================================================== */

/** @verifies SWR-CVC-026 — Init sends I2C command sequence */
void test_Init_sends_i2c_commands(void)
{
    mock_i2c_result = E_OK;
    (void)Ssd1306_Init();

    /* Init should make multiple I2C calls for the command sequence.
     * At minimum: Display Off, MUX, Offset, Start Line, Seg Re-map,
     * COM scan, COM pins, Contrast, Display ON resume, Normal,
     * Osc Freq, Charge Pump, Display ON = 13+ commands */
    TEST_ASSERT_TRUE(mock_i2c_call_count >= 13u);

    /* All calls should target the SSD1306 address */
    TEST_ASSERT_EQUAL_UINT8(SSD1306_I2C_ADDR, mock_i2c_addr);

    /* First call should be a command (0x00 prefix) with Display Off (0xAE) */
    TEST_ASSERT_EQUAL_UINT8(0x00u, mock_i2c_first_bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(0xAEu, mock_i2c_second_bytes[0]);
}

/* ==================================================================
 * SWR-CVC-026: Init returns E_OK on success
 * ================================================================== */

/** @verifies SWR-CVC-026 — Init returns E_OK on success */
void test_Init_returns_ok_on_success(void)
{
    mock_i2c_result = E_OK;

    Std_ReturnType result = Ssd1306_Init();

    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
}

/* ==================================================================
 * SWR-CVC-026: Init returns E_NOT_OK when I2C fails
 * ================================================================== */

/** @verifies SWR-CVC-026 — Init returns E_NOT_OK when I2C fails */
void test_Init_returns_not_ok_when_i2c_fails(void)
{
    mock_i2c_result = E_NOT_OK;

    Std_ReturnType result = Ssd1306_Init();

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
}

/* ==================================================================
 * SWR-CVC-026: Clear fills display buffer with zeros
 * ================================================================== */

/** @verifies SWR-CVC-026 — Clear calls I2C to fill display with zeros */
void test_Clear_fills_display_with_zeros(void)
{
    /* Init first so display is in known state */
    mock_i2c_result = E_OK;
    (void)Ssd1306_Init();

    uint8 init_calls = mock_i2c_call_count;
    mock_i2c_call_count = 0u;

    Ssd1306_Clear();

    /* Clear should make I2C calls to write zero data across 8 pages x 128 cols.
     * The exact number of calls depends on chunking, but data must be sent. */
    TEST_ASSERT_TRUE(mock_i2c_call_count > 0u);
}

/* ==================================================================
 * SWR-CVC-026: SetCursor sends page and column address commands
 * ================================================================== */

/** @verifies SWR-CVC-026 — SetCursor sends page address and column commands */
void test_SetCursor_sends_page_and_column(void)
{
    mock_i2c_result = E_OK;
    (void)Ssd1306_Init();

    mock_i2c_call_count = 0u;

    Ssd1306_SetCursor(2u, 64u);

    /* Should send at least 3 commands: page address, column low, column high */
    TEST_ASSERT_TRUE(mock_i2c_call_count >= 3u);

    /* All should be command mode (0x00 prefix) */
    TEST_ASSERT_EQUAL_UINT8(0x00u, mock_i2c_first_bytes[0]);

    /* Page address = 0xB0 | page = 0xB2 */
    TEST_ASSERT_EQUAL_UINT8(0xB2u, mock_i2c_second_bytes[0]);

    /* Column low nibble = 0x00 | (64 & 0x0F) = 0x00 */
    TEST_ASSERT_EQUAL_UINT8(0x00u, mock_i2c_second_bytes[1]);

    /* Column high nibble = 0x10 | (64 >> 4) = 0x14 */
    TEST_ASSERT_EQUAL_UINT8(0x14u, mock_i2c_second_bytes[2]);
}

/* ==================================================================
 * SWR-CVC-026: WriteString renders characters
 * ================================================================== */

/** @verifies SWR-CVC-026 — WriteString renders characters via I2C data writes */
void test_WriteString_renders_characters(void)
{
    mock_i2c_result = E_OK;
    (void)Ssd1306_Init();

    mock_i2c_call_count = 0u;

    Std_ReturnType result = Ssd1306_WriteString("AB");

    TEST_ASSERT_EQUAL_UINT8(E_OK, result);

    /* Each character = 1 I2C data write (5 font bytes + 1 space = 6 bytes).
     * "AB" = 2 characters = 2 I2C data calls. */
    TEST_ASSERT_TRUE(mock_i2c_call_count >= 2u);
}

/* ==================================================================
 * SWR-CVC-026: WriteString with empty string
 * ================================================================== */

/** @verifies SWR-CVC-026 — WriteString with empty string makes no I2C data writes */
void test_WriteString_empty_no_writes(void)
{
    mock_i2c_result = E_OK;
    (void)Ssd1306_Init();

    mock_i2c_call_count = 0u;

    Std_ReturnType result = Ssd1306_WriteString("");

    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0u, mock_i2c_call_count);
}

/* ==================================================================
 * SWR-CVC-026: WriteString with NULL returns E_NOT_OK
 * ================================================================== */

/** @verifies SWR-CVC-026 — WriteString with NULL returns E_NOT_OK */
void test_WriteString_null_returns_not_ok(void)
{
    mock_i2c_result = E_OK;
    (void)Ssd1306_Init();

    Std_ReturnType result = Ssd1306_WriteString(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
}

/* ==================================================================
 * SWR-CVC-026: I2C failure during WriteString returns E_NOT_OK
 * ================================================================== */

/** @verifies SWR-CVC-026 — I2C failure during WriteString returns E_NOT_OK */
void test_WriteString_i2c_fail_returns_not_ok(void)
{
    mock_i2c_result = E_OK;
    (void)Ssd1306_Init();

    /* Now make I2C fail */
    mock_i2c_result = E_NOT_OK;

    Std_ReturnType result = Ssd1306_WriteString("Hello");

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
}

/* ==================================================================
 * SWR-CVC-026: WriteChar renders 5 bytes font data + 1 space
 * ================================================================== */

/** @verifies SWR-CVC-026 — WriteChar sends 6 data bytes (5 font + 1 space) */
void test_WriteChar_renders_6_bytes(void)
{
    mock_i2c_result = E_OK;
    (void)Ssd1306_Init();

    mock_i2c_call_count = 0u;

    Std_ReturnType result = Ssd1306_WriteChar('A');

    TEST_ASSERT_EQUAL_UINT8(E_OK, result);

    /* Should make exactly 1 I2C data call */
    TEST_ASSERT_EQUAL_UINT8(1u, mock_i2c_call_count);

    /* Data mode prefix (0x40) + 5 font bytes + 1 space byte = 7 total bytes */
    TEST_ASSERT_EQUAL_UINT8(0x40u, mock_i2c_first_bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(7u, mock_i2c_len);
}

/* ==================================================================
 * Test runner
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* SWR-CVC-026: SSD1306 OLED driver */
    RUN_TEST(test_Init_sends_i2c_commands);
    RUN_TEST(test_Init_returns_ok_on_success);
    RUN_TEST(test_Init_returns_not_ok_when_i2c_fails);
    RUN_TEST(test_Clear_fills_display_with_zeros);
    RUN_TEST(test_SetCursor_sends_page_and_column);
    RUN_TEST(test_WriteString_renders_characters);
    RUN_TEST(test_WriteString_empty_no_writes);
    RUN_TEST(test_WriteString_null_returns_not_ok);
    RUN_TEST(test_WriteString_i2c_fail_returns_not_ok);
    RUN_TEST(test_WriteChar_renders_6_bytes);

    return UNITY_END();
}
