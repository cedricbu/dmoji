
TARGET   = dmoji
DESTDIR ?= 
PREFIX  ?= /usr/local
BINDIR  ?= /bin
SOURCE   = dmoji.c
CC      ?= $(shell icu-config --cc)
LDFLAGS  =  $(shell icu-config --ldflags --ldflags-icuio)

$(TARGET): $(SOURCE)
	icu-config --exists
	$(CC) $(LDFLAGS) $(SOURCE) -o $(TARGET)

debug:
	$(CC) $(LDFLAGS) -DDEBUG $(SOURCE) -o $(TARGET)

install:
	install -d $(DESTDIR)$(PREFIX)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)$(BINDIR)/$(TARGET)

uninstall:
	echo $(DESTDIR)$(PREFIX)$(BINDIR)/$(TARGET)
	#$(RM) $(DESTDIR)$(PREFIX)$(BINDIR)/$(TARGET)

clean:
	$(RM) $(TARGET)


.PHONY: debug clean uninstall install
