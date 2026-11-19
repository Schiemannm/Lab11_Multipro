CC=gcc
CFLAGS=-c -Wall -g
LDFLAGS=-ljpeg -lm

MANDEL_SOURCES=mandel.c jpegrw.c
MOVIE_SOURCES=mandelmovie.c
SOURCES=$(MANDEL_SOURCES) $(MOVIE_SOURCES)

MANDEL_OBJECTS=$(MANDEL_SOURCES:.c=.o)
MOVIE_OBJECTS=$(MOVIE_SOURCES:.c=.o)
OBJECTS=$(SOURCES:.c=.o)

EXECUTABLES=mandel mandelmovie

all: $(SOURCES) $(EXECUTABLES)

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

mandel: $(MANDEL_OBJECTS)
	$(CC) $(MANDEL_OBJECTS) $(LDFLAGS) -o $@

mandelmovie: $(MOVIE_OBJECTS)
	$(CC) $(MOVIE_OBJECTS) -lm -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
	$(CC) -MM $< > $*.d

clean:
	rm -rf $(OBJECTS) $(EXECUTABLES) *.d

# remove generated frames and movies
clean-frames:
	rm -f mandel*.jpg *.mpg *.mp4

.PHONY: all clean clean-frames