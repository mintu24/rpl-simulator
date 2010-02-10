
NODES = $(subst .svg,.png,$(wildcard node-*-*-*.svg))
PATTERNS = $(subst .svg,.png,$(wildcard *-pattern.svg))
RSVG = rsvg -f png
RM = rm -f

all: $(NODES) $(PATTERNS)

clean:
	$(RM) $(NODES)

%.png: %.svg
	w=`cat $^ | grep -m 1 '^\( \)\+width' | cut -d '=' -f 2 | tr -d '"'`; \
	h=`cat $^ | grep -m 1 '^\( \)\+height' | cut -d '=' -f 2 | tr -d '"'`; \
    $(RSVG) -w $$w -h $$h $^ $@

