CFLAGS = -g -Wall -Wextra -pedantic-errors

DEST = build
OBJS = $(addprefix $(DEST)/, wordcounter.so mapreduce-simple mapreduce-server)

DEPS     = $(addprefix $(DEST)/, mapreduce-utils.o)
RPC_DEPS = $(addprefix rpc/$(DEST)/, server.o client.o pack.o utils.o)

all: rpc $(OBJS)

rpc:
	$(MAKE) DEST='$(DEST)' CFLAGS='$(CFLAGS)' -C rpc

$(DEST):
	mkdir -p $(DEST)

$(DEST)/mapreduce-utils.o: mapreduce-utils.c mapreduce-utils.h | $(DEST)
	$(CC) $(CFLAGS) -o $@ -c mapreduce-utils.c

# should not link with mapreduce-utils.o, causes error:
# warning: relocation against `stderr@@GLIBC_2.2.5' in read-only section `.text'
# relocation R_X86_64_PC32 against symbol `stderr@@GLIBC_2.2.5' can not be used when making a shared object; recompile with -fPIC
$(DEST)/wordcounter.so: wordcounter.c wordcounter.h | $(DEST)
	$(CC) $(CFLAGS) -shared -o $@ wordcounter.c

$(DEST)/mapreduce-%: mapreduce-%.c $(DEPS) $(RPC_DEPS) | $(DEST)
	$(CC) $(CFLAGS) -rdynamic -o $@ $^ -ldl

clean:
	rm -f $(DEPS) $(OBJS)
	rmdir $(DEST)

	$(MAKE) DEST='$(DEST)' -C rpc clean

.PHONY: clean rpc
