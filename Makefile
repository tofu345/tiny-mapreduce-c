CFLAGS = -g -Wall -Wextra -pedantic-errors

DEST = out
DEPS = $(addprefix $(DEST)/, mapreduce-utils.o)
OBJS = $(addprefix $(DEST)/, wordcounter.so mapreduce-simple mapreduce-server)

all: rpc $(OBJS)

$(DEST):
	mkdir -p $(DEST)

$(DEST)/mapreduce-utils.o: mapreduce-utils.c mapreduce-utils.h | $(DEST)
	$(CC) $(CFLAGS) -o $@ -c mapreduce-utils.c

$(DEST)/wordcounter.so: wordcounter.h wordcounter.c $(DEPS) | $(DEST)
	$(CC) $(CFLAGS) -shared -o $@ wordcounter.c $(DEPS)

$(DEST)/mapreduce-%: mapreduce-%.c $(DEPS) | $(DEST)
	$(CC) $(CFLAGS) -rdynamic -o $@ $< $(DEPS) -ldl

rpc:
	$(MAKE) CFLAGS="$(CFLAGS)" -C rpc

clean:
	rm -f $(DEPS) $(OBJS)
	rmdir $(DEST)

	$(MAKE) -C rpc clean

.PHONY: clean rpc
