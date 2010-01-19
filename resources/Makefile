
NODES = $(subst .svg,.png,$(wildcard node-*.svg))
RSVG = rsvg -f png
RM = rm -f

all: $(NODES)

clean:
	$(RM) $(NODES)

%.png: %.svg
	w=`cat $^ | grep '^\( \)\+width' | cut -d '=' -f 2 | tr -d '"'`; \
	h=`cat $^ | grep '^\( \)\+height' | cut -d '=' -f 2 | tr -d '"'`; \
    $(RSVG) -w $$w -h $$h $^ $@

