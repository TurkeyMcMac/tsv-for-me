name = tsvfm
version = 1.1.2
flags = -O3 -Wall $(CFLAGS)
bin = ~/bin
man = /usr/local/man/man1

$(name): tsv-for-me.c
	$(CC) $(flags) -DVERSION='"$(version)"' -o $(name) $<

.PHONY: install
install: $(name) $(name).1
	sudo ln -f $(name) $(bin)
	sudo mkdir -p $(man)
	sudo cp $(name).1 $(man)
	sudo gzip -f $(man)/$(name).1

.PHONY: uninstall
uninstall:
	sudo $(RM) $(bin)/$(name) $(man)/$(name).1

.PHONY: clean
clean:
	$(RM) $(name)
