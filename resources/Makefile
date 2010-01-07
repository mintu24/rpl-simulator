
NODES = $(subst .svg,.png,$(wildcard node-*.svg))
RSVG = rsvg -f png -w 128 -h 128
RM = rm -f

all: $(NODES)

clean:
	$(RM) $(NODES)

%.png: %.svg
	$(RSVG) $(subst .png,.svg,$@) $@
