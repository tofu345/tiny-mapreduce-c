CFLAGS = -g -Wall -Wextra -pedantic-errors

DEST = out
DEPS = $(addprefix $(DEST)/, pack.o utils.o client.o server.o)
BINS = $(addprefix $(DEST)/, test_pack test_rpc test_client test_server)

all: $(BINS)

$(DEST)/test_rpc: test_rpc.c $(DEPS)
	$(CC) $(CFLAGS) -o $@ $^

$(DEST)/test_pack: test_pack.c $(addprefix $(DEST)/, pack.o utils.o)
	$(CC) $(CFLAGS) -o $@ $^

$(DEST)/test_client: test_client.c $(addprefix $(DEST)/, pack.o utils.o client.o)
	$(CC) $(CFLAGS) -o $@ $^

$(DEST)/test_server: test_server.c $(addprefix $(DEST)/, pack.o utils.o server.o)
	$(CC) $(CFLAGS) -o $@ $^

$(DEST):
	mkdir -p $(DEST)

$(DEST)/%.o: %.c %.h | $(DEST)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(DEPS) $(BINS)
	rmdir $(DEST)
