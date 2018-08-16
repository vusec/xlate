BUILD ?= obj

all: $(BUILD)/histogram

all: $(BUILD)/aes-fr
all: $(BUILD)/aes-ff
all: $(BUILD)/aes-pp
all: $(BUILD)/aes-pa
all: $(BUILD)/aes-xp
all: $(BUILD)/aes-xa

all: $(BUILD)/xfer
all: $(BUILD)/count

ARCH ?= $(shell uname -m | \
	sed 's/aarch64/arm64/g' | \
	sed 's/armv7l/arm/g' | \
	sed 's/i[3-6]86/x86/g' | \
	sed 's/x86_64/x86-64/g' | \
	sed 's/amd64/x86-64/g')

CFLAGS += -D_GNU_SOURCE -g3 -Wall -Wextra -std=gnu11 -O2
CFLAGS += -Iinclude
LDFLAGS += -flto -O2
LIBS += -lelf -lpthread -lrt

-include source/Makefile

dep = $(obj:.o=.d)
-include $(dep)

.PHONY: clean all

obj = $(addprefix $(BUILD)/, $(obj-y))

obj-histogram = $(addprefix $(BUILD)/, $(obj-histogram-y))
obj-histogram += $(obj)

$(BUILD)/histogram: $(obj-histogram)
	@echo "LD $@"
	@mkdir -p $(dir $@)
	@$(CC) $(obj-histogram) -o $@ $(LDFLAGS) $(LIBS)

obj-histogram-pp = $(addprefix $(BUILD)/, $(obj-histogram-pp-y))
obj-histogram-pp += $(obj)

$(BUILD)/histogram-pp: $(obj-histogram-pp)
	@echo "LD $@"
	@mkdir -p $(dir $@)
	@$(CC) $(obj-histogram-pp) -o $@ $(LDFLAGS) $(LIBS)

obj-histogram-xp = $(addprefix $(BUILD)/, $(obj-histogram-xp-y))
obj-histogram-xp += $(obj)

$(BUILD)/histogram-xp: $(obj-histogram-xp)
	@echo "LD $@"
	@mkdir -p $(dir $@)
	@$(CC) $(obj-histogram-xp) -o $@ $(LDFLAGS) $(LIBS)

obj-aes-fr = $(addprefix $(BUILD)/, $(obj-aes-fr-y))
obj-aes-fr += $(obj)

$(BUILD)/aes-fr: LDFLAGS += -Lopenssl-1.0.1e
$(BUILD)/aes-fr: LIBS += -lcrypto
$(BUILD)/aes-fr: $(obj-aes-fr)
	@echo "LD $@"
	@mkdir -p $(dir $@)
	@$(CC) $(obj-aes-fr) -o $@ $(LDFLAGS) $(LIBS)

obj-aes-ff = $(addprefix $(BUILD)/, $(obj-aes-ff-y))
obj-aes-ff += $(obj)

$(BUILD)/aes-ff: LDFLAGS += -Lopenssl-1.0.1e
$(BUILD)/aes-ff: LIBS += -lcrypto
$(BUILD)/aes-ff: $(obj-aes-ff)
	@echo "LD $@"
	@mkdir -p $(dir $@)
	@$(CC) $(obj-aes-ff) -o $@ $(LDFLAGS) $(LIBS)

obj-aes-pp = $(addprefix $(BUILD)/, $(obj-aes-pp-y))
obj-aes-pp += $(obj)

$(BUILD)/aes-pp: LDFLAGS += -Lopenssl-1.0.1e
$(BUILD)/aes-pp: LIBS += -lcrypto
$(BUILD)/aes-pp: $(obj-aes-pp)
	@echo "LD $@"
	@mkdir -p $(dir $@)
	@$(CC) $(obj-aes-pp) -o $@ $(LDFLAGS) $(LIBS)

obj-aes-pa = $(addprefix $(BUILD)/, $(obj-aes-pa-y))
obj-aes-pa += $(obj)

$(BUILD)/aes-pa: LDFLAGS += -Lopenssl-1.0.1e
$(BUILD)/aes-pa: LIBS += -lcrypto
$(BUILD)/aes-pa: $(obj-aes-pa)
	@echo "LD $@"
	@mkdir -p $(dir $@)
	@$(CC) $(obj-aes-pa) -o $@ $(LDFLAGS) $(LIBS)

obj-aes-xp = $(addprefix $(BUILD)/, $(obj-aes-xp-y))
obj-aes-xp += $(obj)

$(BUILD)/aes-xp: LDFLAGS += -Lopenssl-1.0.1e
$(BUILD)/aes-xp: LIBS += -lcrypto
$(BUILD)/aes-xp: $(obj-aes-xp)
	@echo "LD $@"
	@mkdir -p $(dir $@)
	@$(CC) $(obj-aes-xp) -o $@ $(LDFLAGS) $(LIBS)

obj-aes-xa = $(addprefix $(BUILD)/, $(obj-aes-xa-y))
obj-aes-xa += $(obj)

$(BUILD)/aes-xa: LDFLAGS += -Lopenssl-1.0.1e
$(BUILD)/aes-xa: LIBS += -lcrypto
$(BUILD)/aes-xa: $(obj-aes-xa)
	@echo "LD $@"
	@mkdir -p $(dir $@)
	@$(CC) $(obj-aes-xa) -o $@ $(LDFLAGS) $(LIBS)

obj-xfer = $(addprefix $(BUILD)/, $(obj-xfer-y))
obj-xfer += $(obj)

$(BUILD)/xfer: LDFLAGS += -Lopenssl-1.0.1e
$(BUILD)/xfer: LIBS += -lcrypto
$(BUILD)/xfer: $(obj-xfer)
	@echo "LD $@"
	@mkdir -p $(dir $@)
	@$(CC) $(obj-xfer) -o $@ $(LDFLAGS) $(LIBS)

obj-count = $(addprefix $(BUILD)/, $(obj-count-y))
obj-count += $(obj)

$(BUILD)/count: LDFLAGS += -Lopenssl-1.0.1e
$(BUILD)/count: LIBS += -lcrypto
$(BUILD)/count: $(obj-count)
	@echo "LD $@"
	@mkdir -p $(dir $@)
	@$(CC) $(obj-count) -o $@ $(LDFLAGS) $(LIBS)

$(BUILD)/%.o: %.c
	@echo "CC $<"
	@mkdir -p $(dir $@)
	@$(CC) -c $< -o $@ $(CFLAGS) -MT $@ -MMD -MP -MF $(@:.o=.d)

clean:
	@rm -rf $(BUILD)
