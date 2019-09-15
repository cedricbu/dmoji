


GIT_VERSION := $(shell git --no-pager describe --tags --always)
GIT_COMMIT  := $(shell git rev-parse --verify HEAD)


TARGET   = dmoji
DESTDIR ?= 
PREFIX  ?= /usr/local
BINDIR  ?= /bin
SOURCE   = dmoji.c
CC      ?= $(shell icu-config --cc)
LDFLAGS += $(shell icu-config --ldflags --ldflags-icuio)



CFLAGS  += -DGIT_VERSION=\"$(GIT_VERSION)\" -DGIT_COMMIT=\"$(GIT_COMMIT)\" -DTARGET=\"$(TARGET)\"

$(TARGET): $(SOURCE)
	icu-config --exists
	$(CC) $(LDFLAGS) $(SOURCE) $(CFLAGS) -o $(TARGET)

debug:
	$(CC) $(LDFLAGS) -DDEBUG -fsanitize=address $(CFLAGS) $(SOURCE) -o $(TARGET)

install:
	install -d $(DESTDIR)$(PREFIX)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)$(BINDIR)/$(TARGET)

uninstall:
	echo $(DESTDIR)$(PREFIX)$(BINDIR)/$(TARGET)
	#$(RM) $(DESTDIR)$(PREFIX)$(BINDIR)/$(TARGET)

clean:
	$(RM) $(TARGET)


.PHONY: debug clean uninstall install
