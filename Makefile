name = tsvfm
version = 1.1.0
flags = -O3 -Wall $(CFLAGS)

$(name): tsv-for-me.c
	$(CC) $(flags) -DVERSION='"$(version)"' -o $(name) $<

.PHONY: clean
clean:
	$(RM) $(name)
