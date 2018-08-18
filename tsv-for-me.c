#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct string {
	char *chars;
	size_t length;
};

ssize_t get_columns(FILE *from, struct string **into, size_t *capacity);
void get_widths(const struct string *from, size_t *into, size_t n_columns);
void fit_row(struct string **cells, size_t *capacity, size_t grow_to);
ssize_t get_row(FILE *from, struct string *into, size_t max_columns);
#define MAX(a, b) ( a > b ? a : b )
int print_row(const struct string *row, const size_t *widths, size_t n_columns);
int print_separator(const size_t *widths, size_t n_columns);

int main(void)
{
	struct string *cells;
	size_t cell_cap;
	ssize_t n_columns = get_columns(stdin, &cells, &cell_cap);
	if (n_columns < 0) {
		return 1;
	}
	size_t *widths = malloc(n_columns * sizeof(*widths));
	get_widths(cells, widths, n_columns);

	fit_row(&cells, &cell_cap, n_columns * 2);
	struct string empty_string = {"", 0};
	ssize_t rowsize;
	size_t n_cells;
	for (size_t r = n_columns;
	     n_cells = r, (rowsize = get_row(stdin, &cells[r], n_columns)) >= 0;
	     r += n_columns, fit_row(&cells, &cell_cap, r + n_columns))
	{
		for (size_t i = 0; i < rowsize; ++i) {
			widths[i] = MAX(widths[i], cells[r + i].length);
		}
		while (rowsize < n_columns) {
			cells[r + rowsize] = empty_string;
			++rowsize;
		}
	}

	for (size_t i = 0; i < n_columns; ++i) {
		widths[i] += 2;
	}
	print_row(cells, widths, n_columns);
	print_separator(widths, n_columns);
	for (size_t r = n_columns; r < n_cells; r += n_columns) {
		print_row(&cells[r], widths, n_columns);
	}
}

struct item_iter {
	char *line;
	struct string *into;
	size_t length;
	size_t idx;
	size_t base;
	size_t line_idx;
};

#define ITEM_ITER(line_, into_) {					\
	.length = 0,							\
	.idx = 0,							\
	.base = 0,							\
	.line_idx = 0,							\
	.line = (line_),						\
	.into = (into_)							\
}

void item_iter_next(struct item_iter *iter);

ssize_t get_columns(FILE *from, struct string **into, size_t *capacity)
{
	char *line = NULL;
	size_t line_capacity = 0;
	ssize_t line_length = getline(&line, &line_capacity, from);
	if (line_length < 0) {
		free(line);
		return -1;
	}
	*capacity = line_capacity * sizeof(**into);
	*into = malloc(*capacity);
	struct item_iter item = ITEM_ITER(line, *into);
	while (item.line_idx < line_length) {
		item_iter_next(&item);
	}
	return item.idx;
}

void get_widths(const struct string *from, size_t *into, size_t n_columns)
{
	while (n_columns--) {
		into[n_columns] = from[n_columns].length;
	}
}

void fit_row(struct string **cells, size_t *capacity, size_t grow_to)
{
	if (grow_to * sizeof(**cells) <= *capacity) {
		return;
	}
	*capacity *= 2;
	struct string *new_cells = realloc(*cells, *capacity);
	*cells = new_cells;
}

ssize_t get_row(FILE *from, struct string *into, size_t max_columns)
{
	char *line = NULL;
	size_t line_capacity = 0;
	ssize_t line_length = getline(&line, &line_capacity, from);
	if (line_length < 0) {
		free(line);
		return -1;
	}
	struct item_iter item = ITEM_ITER(line, into);
	while (item.line_idx < line_length && item.idx < max_columns) {
		item_iter_next(&item);
	}
	return item.idx;
}

void item_iter_next(struct item_iter *iter)
{
	char ch = iter->line[iter->line_idx];
	if (ch == '\t' || ch == '\n') {
		iter->into[iter->idx].chars = &iter->line[iter->base];
		iter->into[iter->idx].length = iter->length;
		iter->length = 0;
		iter->base = iter->line_idx + 1;
		++iter->idx;
		iter->line[iter->line_idx] = '\0';
	} else if (!(ch & 0x80)) {
		++iter->length;
	} else {
		unsigned ch_int =
			(unsigned)ch << ((sizeof(unsigned) - 1) * 8);
		size_t ch_length = __builtin_clz(~ch_int);
		iter->line_idx += ch_length;
		++iter->length;
		return;
	}
	++iter->line_idx;
}

int print_row(const struct string *row, const size_t *widths, size_t n_columns)
{
	for (size_t i = 0; i < n_columns; ++i) {
		printf("%s%*s",
			row[i].chars, (int)(widths[i] - row[i].length), "");
	}
	printf("\n");
	return 0;
}

int print_separator(const size_t *widths, size_t n_columns)
{
	size_t total_width = 0;
	for (size_t i = 0; i < n_columns; ++i) {
		total_width += widths[i];
	}
	char *buf = malloc(total_width + 1);
	memset(buf, '-', total_width);
	buf[total_width] = '\0';
	printf("%s\n", buf);
	free(buf);
	return 0;
}
