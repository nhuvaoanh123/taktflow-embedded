---
paths:
  - "firmware/**/*.c"
  - "firmware/**/*.h"
  - "firmware/**/*.cpp"
  - "firmware/**/*.hpp"
---

# Code Style Standards (C/C++ Embedded)

## Naming Conventions

| Element | Style | Example |
|---------|-------|---------|
| Functions | `snake_case` | `uart_send_byte()` |
| Variables (local) | `snake_case` | `retry_count` |
| Variables (global) | `g_snake_case` | `g_system_state` |
| Constants | `UPPER_SNAKE_CASE` | `MAX_BUFFER_SIZE` |
| Macros | `UPPER_SNAKE_CASE` | `ARRAY_SIZE(x)` |
| Types (struct/enum) | `snake_case_t` | `sensor_data_t` |
| Enum values | `UPPER_SNAKE_CASE` | `STATE_IDLE` |
| Header guards | `FILENAME_H` | `UART_DRIVER_H` |
| Module prefix | `module_` | `mqtt_connect()`, `ble_init()` |

## File Organization

```c
// 1. File header comment
// 2. Includes (system, then project, then local)
// 3. Preprocessor defines
// 4. Type definitions (typedef, struct, enum)
// 5. Static (private) function prototypes
// 6. Global variables (minimize these)
// 7. Static (private) variables
// 8. Public function implementations
// 9. Static (private) function implementations
```

## Include Order

```c
#include <stdlib.h>          // 1. System/standard headers
#include <string.h>

#include "freertos/FreeRTOS.h" // 2. RTOS / framework headers

#include "hal_uart.h"        // 3. Project HAL headers
#include "mqtt_client.h"     // 4. Project module headers

#include "this_module.h"     // 5. This module's own header (last)
```

## Formatting

- Indentation: 4 spaces (no tabs)
- Brace style: Allman (opening brace on new line) for functions, K&R for control flow
- Maximum line length: 100 characters
- One statement per line
- One variable declaration per line
- Blank line between logical sections within a function

```c
// K&R braces for control flow:
if (condition) {
    do_thing();
} else {
    do_other_thing();
}

// Allman braces for functions:
error_t mqtt_connect(const char *host, uint16_t port)
{
    // function body
}
```

## Variable Scope

- Declare variables in the smallest possible scope
- Prefer local variables over global
- If global is needed, make it `static` (file-scope) unless truly shared
- Extern globals must be declared in a header file, defined in one source file

## Magic Numbers

- No magic numbers in code — use named constants
- `#define` or `const` or `enum` depending on context
- Exception: 0, 1, -1 in obvious contexts (loop init, null check)

```c
// BAD:
if (retry_count > 5) { ... }
delay_ms(3000);

// GOOD:
#define MAX_RETRIES 5
#define RECONNECT_DELAY_MS 3000
if (retry_count > MAX_RETRIES) { ... }
delay_ms(RECONNECT_DELAY_MS);
```

## Struct Initialization

- Always use designated initializers for clarity
- Initialize all fields explicitly — don't rely on zero-init

```c
sensor_config_t config = {
    .pin = PIN_SENSOR_ADC,
    .sample_rate_hz = 100,
    .filter_type = FILTER_MOVING_AVG,
    .filter_window = 8,
};
```
