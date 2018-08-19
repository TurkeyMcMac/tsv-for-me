name = tsvfm
version = 1.1.1
flags = -O3 -Wall $(CFLAGS)
bin = ~/bin

$(name): tsv-for-me.c
	$(CC) $(flags) -DVERSION='"$(version)"' -o $(name) $<

.PHONY: install
install: $(name)
	sudo ln -f $(name) $(bin)

.PHONY: uninstall
uninstall: $(name)
	sudo $(RM) $(bin)/$(name)

.PHONY: clean
clean:
	$(RM) $(name)
