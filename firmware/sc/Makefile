# SC Safety Controller — TMS570LC43x Target Build
#
# Matches CCS project (Taktflow_SC) build configuration exactly:
#   - tiarmclang v4.0.4.LTS
#   - Original HALCoGen .asm files via -xti-asm
#   - --rom_model --be32 linker flags
#   - libc.a linked
#
# Usage:
#   make              — build sc.out (debug, no optimization)
#   make release      — build sc.out (optimized)
#   make flash        — flash via DSLite
#   make clean        — remove build artifacts

# ---- Toolchain ----
CGT_ROOT  := C:/ti/ccs2041/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS
CC        := $(CGT_ROOT)/bin/tiarmclang.exe
LNK       := $(CGT_ROOT)/bin/tiarmclang.exe

# ---- Paths ----
SC_SRC    := src
SC_INC    := include
HCG_ROOT  := ../../halcogencfg/TMS570LC4357
HCG_SRC   := $(HCG_ROOT)/source
HCG_INC   := $(HCG_ROOT)/include
BUILD     := build/tms570

# ---- Compiler Flags (matches CCS project) ----
# TMS570LC43x = ARM Cortex-R5F, big-endian, VFPv3-D16
CFLAGS := -mcpu=cortex-r5 -mbig-endian -mfloat-abi=hard -mfpu=vfpv3-d16 \
          -marm -Og \
          -DPLATFORM_TMS570 -D__big_endian__ \
          -I$(SC_INC) -I$(HCG_INC) -I$(CGT_ROOT)/include \
          -Wall -Wno-unused-parameter \
          -g

# TI-syntax assembly flags (for original HALCoGen .asm files)
AFLAGS := -mcpu=cortex-r5 -mbig-endian -mfloat-abi=hard -mfpu=vfpv3-d16 \
          -marm -Og \
          -DPLATFORM_TMS570 -D__big_endian__ \
          -I$(HCG_INC) -I$(CGT_ROOT)/include \
          -g -xti-asm

# Linker flags (matches CCS project)
LDFLAGS := -mcpu=cortex-r5 -mbig-endian -mfloat-abi=hard -mfpu=vfpv3-d16 \
           -marm \
           -Wl,-m,$(BUILD)/sc.map \
           -Wl,-i$(CGT_ROOT)/lib \
           -Wl,--reread_libs \
           -Wl,--diag_wrap=off \
           -Wl,--display_error_number \
           -Wl,--warn_sections \
           -Wl,--rom_model \
           -Wl,--be32

# Linker script and libraries (must come AFTER object files)
LDPOST := $(HCG_SRC)/HL_sys_link.cmd \
          -Wl,-llibc.a

# ---- SC Application Sources ----
SC_CSRC := $(SC_SRC)/sc_main.c \
           $(SC_SRC)/sc_can.c \
           $(SC_SRC)/sc_e2e.c \
           $(SC_SRC)/sc_esm.c \
           $(SC_SRC)/sc_heartbeat.c \
           $(SC_SRC)/sc_hw_tms570.c \
           $(SC_SRC)/sc_led.c \
           $(SC_SRC)/sc_plausibility.c \
           $(SC_SRC)/sc_relay.c \
           $(SC_SRC)/sc_selftest.c \
           $(SC_SRC)/sc_watchdog.c

# ---- HALCoGen Assembly Sources (original TI-syntax .asm) ----
# These replace our sc_startup.S — tiarmclang supports them via -xti-asm
HCG_ASM := $(HCG_SRC)/HL_sys_intvecs.asm \
           $(HCG_SRC)/HL_sys_core.asm \
           $(HCG_SRC)/HL_sys_mpu.asm \
           $(HCG_SRC)/HL_sys_pmu.asm

# ---- HALCoGen C Sources ----
# EXCLUDED (our sc_hw_tms570.c provides these):
#   HL_gio.c, HL_rti.c, HL_sci.c, HL_notification.c
HCG_CSRC := $(HCG_SRC)/HL_sys_startup.c \
            $(HCG_SRC)/HL_system.c \
            $(HCG_SRC)/HL_sys_vim.c \
            $(HCG_SRC)/HL_sys_pcr.c \
            $(HCG_SRC)/HL_sys_phantom.c \
            $(HCG_SRC)/HL_sys_pmm.c \
            $(HCG_SRC)/HL_sys_dma.c \
            $(HCG_SRC)/HL_pinmux.c \
            $(HCG_SRC)/HL_can.c \
            $(HCG_SRC)/HL_esm.c \
            $(HCG_SRC)/HL_errata.c \
            $(HCG_SRC)/HL_errata_SSWF021_45.c

# ---- Object Files ----
SC_OBJ   := $(patsubst $(SC_SRC)/%.c,$(BUILD)/sc_%.o,$(SC_CSRC))
HCG_AOBJ := $(patsubst $(HCG_SRC)/%.asm,$(BUILD)/hcg_%.o,$(HCG_ASM))
HCG_COBJ := $(patsubst $(HCG_SRC)/%.c,$(BUILD)/hcg_%.o,$(HCG_CSRC))
ALL_OBJ  := $(SC_OBJ) $(HCG_AOBJ) $(HCG_COBJ)

TARGET   := $(BUILD)/sc.out

# ---- Default Target ----
.PHONY: all release flash clean

all: $(TARGET)
	@echo "=== SC build complete: $(TARGET) ==="

release: CFLAGS += -Os
release: $(TARGET)
	@echo "=== SC release build complete: $(TARGET) ==="

$(TARGET): $(ALL_OBJ) | $(BUILD)
	$(LNK) $(LDFLAGS) -o $@ $(ALL_OBJ) $(LDPOST)

# SC application C sources
$(BUILD)/sc_%.o: $(SC_SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

# HALCoGen TI-syntax assembly sources
$(BUILD)/hcg_%.o: $(HCG_SRC)/%.asm | $(BUILD)
	$(CC) $(AFLAGS) -c -o $@ $<

# HALCoGen C sources (relaxed warnings — vendor code)
$(BUILD)/hcg_%.o: $(HCG_SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -Wno-all -c -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

# ---- Flash via DSLite ----
DSLITE := C:/ti/ccs2041/ccs/ccs_base/DebugServer/bin/DSLite.exe
CCXML  := TMS570LC43xx_XDS110.ccxml

flash: $(TARGET)
	"$(DSLITE)" load -c "$(CCXML)" -f "$(TARGET)"

# ---- Clean ----
clean:
	rm -rf $(BUILD)
