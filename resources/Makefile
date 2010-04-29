NODES = $(subst .svg,.png,$(wildcard node-*-*-*.svg))
PATTERNS = $(subst .svg,.png,$(wildcard *-pattern.svg))
RSVG = rsvg -f png
RM = rm -f
SIZE_FACTOR = $(shell if grep ^\#define\ USE_DOCUMENT_COLORS_AND_SIZES ../src/base.h > /dev/null; then echo 2; else echo 1; fi)

all: $(NODES) $(PATTERNS)

clean:
	$(RM) $(NODES) $(PATTERNS)

%.png: %.svg
	w=`cat $^ | grep -m 1 '^\( \)\+width' | cut -d '=' -f 2 | tr -d '"'`; \
	h=`cat $^ | grep -m 1 '^\( \)\+height' | cut -d '=' -f 2 | tr -d '"'`; \
	w=$$(($$w * $(SIZE_FACTOR))); \
	h=$$(($$h * $(SIZE_FACTOR))); \
    $(RSVG) -w $$w -h $$h $^ $@

