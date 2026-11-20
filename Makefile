CC ?= cc
CFLAGS ?= -O2 -std=c99 -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude -Isrc/internal $(shell pkg-config --cflags libcurl)
LDLIBS += $(shell pkg-config --libs libcurl)
SOURCES = src/core/bot.c src/util/json.c src/rest/http.c src/api/messages.c src/api/guilds.c src/api/interactions.c src/gateway/gateway.c
OBJECTS = $(SOURCES:.c=.o)

all: libmayscord.a hello ping

src/%.o: src/%.c include/mayscord.h src/internal/internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

libmayscord.a: $(OBJECTS)
	$(AR) rcs $@ $^

hello: examples/hello.c libmayscord.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $< libmayscord.a $(LDLIBS) -o $@

ping: examples/ping.c libmayscord.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $< libmayscord.a $(LDLIBS) -o $@

test_mayscord: tests/test.c libmayscord.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $< libmayscord.a $(LDLIBS) -o $@

test: test_mayscord
	./test_mayscord

integration:
	python3 tests/integration.py

clean:
	rm -f $(OBJECTS) libmayscord.a hello ping test_mayscord

.PHONY: all test integration clean
