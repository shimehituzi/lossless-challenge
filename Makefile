CC = gcc-9
CFLAG = -O3 -funroll-loops -Wall

EXE = lc2020_encoder lc2020_decoder

all: $(EXE)

lc2020_encoder: encoder.o common.o rc.o
	$(CC) $(CFLAG) -o $@ $^

lc2020_decoder: decoder.o common.o rc.o
	$(CC) $(CFLAG) -o $@ $^

.c.o:
	$(CC) $(CFLAG) -c $<

encoder.o: encoder.c codec.h
decoder.o: decoder.c codec.h
common.o: common.c codec.h
rc.o: rc.c codec.h

clean:
	rm -f $(EXE) *.o *~