CFLAGS = -g -Wall -Wextra -pedantic-errors

DEST  = build
DEPS  = $(addprefix $(DEST)/, pack.o utils.o client.o server.o)
TESTS = $(addprefix $(DEST)/, test_pack test_rpc test_client test_server)

all: $(TESTS)

$(DEST):
	mkdir -p $(DEST)

$(DEST)/%.o: %.c %.h | $(DEST)
	$(CC) $(CFLAGS) -c -o $@ $<

$(DEST)/test_rpc: test_rpc.c $(DEPS)
	$(CC) $(CFLAGS) -o $@ $^

$(DEST)/test_pack: test_pack.c $(addprefix $(DEST)/, pack.o utils.o)
	$(CC) $(CFLAGS) -o $@ $^

$(DEST)/test_client: test_client.c $(addprefix $(DEST)/, pack.o utils.o client.o)
	$(CC) $(CFLAGS) -o $@ $^

$(DEST)/test_server: test_server.c $(addprefix $(DEST)/, pack.o utils.o server.o)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(DEPS) $(TESTS)
	rmdir $(DEST)

.PHONY: clean
