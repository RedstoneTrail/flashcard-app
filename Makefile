all:
	$(CC) -Wl,-rpath . flashcard-app.c -lncursesw -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -lm -L. -llibsql -o flashcard-app

.PHONY: all
