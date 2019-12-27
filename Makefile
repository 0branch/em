SRC = em1.c em2.c
OBJ = $(SRC:.c=.o)
CFLAGS = -g -Wall

em: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJ) em
