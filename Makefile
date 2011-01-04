# exn - dynamic window manager
# See LICENSE file for copyright and license details.

include config.mk

SRC = exn.c
OBJ = ${SRC:.c=.o}

all: options exn

options:
	@echo exn build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

exn: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f exn ${OBJ} exn-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p exn-${VERSION}
	@cp -R LICENSE Makefile README config.def.h config.mk \
		exn.1 ${SRC} exn-${VERSION}
	@tar -cf exn-${VERSION}.tar exn-${VERSION}
	@gzip exn-${VERSION}.tar
	@rm -rf exn-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f exn ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/exn
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < exn.1 > ${DESTDIR}${MANPREFIX}/man1/exn.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/exn.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/exn
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/exn.1

.PHONY: all options clean dist install uninstall
