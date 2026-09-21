CFLAGS = -g -Wall -Wextra -pedantic-errors

DEST = out
OBJS = $(addprefix $(DEST)/, mapreduce-utils.o)

all: wordcounter mapreduce-simple

$(DEST):
	mkdir -p $(DEST)

$(DEST)/%.o: %.c %.h | $(DEST)
	$(CC) $(CFLAGS) -o $@ -c $< -ldl

wordcounter: wordcounter.h wordcounter.c $(OBJS) | $(DEST)
	$(CC) $(CFLAGS) -shared -o $(DEST)/wordcounter.so wordcounter.c $(OBJS)

mapreduce-simple: mapreduce-simple.c $(OBJS) | $(DEST)
	$(CC) $(CFLAGS) -rdynamic -o $(DEST)/mapreduce-simple mapreduce-simple.c $(OBJS) -ldl

.PHONY: clean
clean:
	rm -f $(OBJS) $(DEST)/wordcounter.so $(DEST)/mapreduce-simple
