EXES = 32/vsthost32.exe 64/vsthost64.exe

BINS32 = /usr/i686-w64-mingw32/bin
BINS64 = /usr/x86_64-w64-mingw32/bin

DEPS32 = 32/libstdc++-6.dll 32/libwinpthread-1.dll 32/libgcc_s_dw2-1.dll
DEPS64 = 64/libstdc++-6.dll 64/libwinpthread-1.dll 64/libgcc_s_seh-1.dll

DEPS = $(DEPS32) $(DEPS64)

CC32 = i686-w64-mingw32-g++
CC64 = x86_64-w64-mingw32-g++

STRIP32 = i686-w64-mingw32-strip
STRIP64 = x86_64-w64-mingw32-strip

WINDRES32 = i686-w64-mingw32-windres
WINDRES64 = x86_64-w64-mingw32-windres

CFLAGS = -O2 -DUNICODE -D_UNICODE

VSTHOST32_OBJS  = vsthost.32.o vsthost.res.32.o

VSTHOST64_OBJS  = vsthost.64.o vsthost.res.64.o

.PHONY: all clean

all: $(EXES) $(DEPS)

32/vsthost32.exe: $(VSTHOST32_OBJS)
	mkdir -p 32
	$(CC32) $^ -o $@ -lole32 -lcomctl32 -Wl,--subsystem,windows
	$(STRIP32) $@

32/libstdc++-6.dll: $(BINS32)/libstdc++-6.dll
	cp $^ $@

32/libwinpthread-1.dll: $(BINS32)/libwinpthread-1.dll
	cp $^ $@

32/libgcc_s_dw2-1.dll: $(BINS32)/libgcc_s_dw2-1.dll
	cp $^ $@

vsthost.32.o: vsthost.cpp stdafx.h
	$(CC32) -c $(CFLAGS) vsthost.cpp -o $@

vsthost.res.32.o: vsthost.rc resource.h
	$(WINDRES32) vsthost.rc -o $@

64/vsthost64.exe: $(VSTHOST64_OBJS)
	mkdir -p 64
	$(CC64) $^ -o $@ -lole32 -lcomctl32 -Wl,--subsystem,windows
	$(STRIP64) $@

64/libstdc++-6.dll: $(BINS64)/libstdc++-6.dll
	cp $^ $@

64/libwinpthread-1.dll: $(BINS64)/libwinpthread-1.dll
	cp $^ $@

64/libgcc_s_seh-1.dll: $(BINS64)/libgcc_s_seh-1.dll
	cp $^ $@

vsthost.64.o: vsthost.cpp stdafx.h
	$(CC64) -c $(CFLAGS) vsthost.cpp -o $@

vsthost.res.64.o: vsthost.rc resource.h
	$(WINDRES64) vsthost.rc -o $@

clean:
	rm -rf 32 64 $(VSTHOST32_OBJS) $(VSTHOST64_OBJS) > /dev/null
