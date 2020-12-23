version = $(shell git describe --dirty || echo dev)
curl=no

ifneq ($(curl),no)
	flags=-DUSE_CURL -lcurl
	LDFLAGS="-R/usr/local/lib -L/usr/local/lib"
else
	flags=
	LDFLAGS=
endif

all: push.so

push.so: push.cpp
	env LDFLAGS="$(LDFLAGS)" CXXFLAGS="$(CXXFLAGS) -DPUSHVERSION=\"\\\"$(version)\\\"\" $(flags)" LIBS="$(LIBS) $(flags)" znc-buildmod push.cpp

install: push.so
	mkdir -p $(HOME)/.znc/modules/
	cp push.so $(HOME)/.znc/modules/push.so

clean:
	-rm -f push.so
