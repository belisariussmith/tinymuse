CC =  gcc
WARNINGS = -Wall
CFLAGS = $(WARNINGS) -g -fpcc-struct-return -Wpointer-arith
TOPDIR = ../
LIBS = -L/usr/local/lib 
COMM = comm.o comm.admin.o comm.create.o comm.dbtop.o comm.help.o comm.info.o comm.look.o comm.pcmds.o comm.player.o comm.rob.o comm.set.o comm.speech.o
DB = db.o db.boolexp.o db.destroy.o db.inherit.o db.mail.o db.warnings.o
IO = io.bsd.o io.log.o io.newconc.o
PROG = prog.ansi.o prog.ctrl.o prog.eval.o prog.hash.o
IDENT = ident.o ident.id_close.o ident.id_open.o ident.id_parse.o ident.id_query.o ident.support.o ident.version.o

all: tinymuse

clean:
	rm -f *.o *.a tinymuse

install: all
	cp tinymuse ../bin/

tinymuse: cque.o class.o game.o match.o move.o nalloc.o players.o powerlist.o powers.o predicates.o stringutil.o timer.o unparse.o utils.o wild.o config.o $(COMM) $(DB) $(IO) $(PROG) $(IDENT)
	$(CC) -o tinymuse -Wall -g -fpcc-struct-return -Wpointer-arith cque.o class.o game.o match.o move.o nalloc.o players.o powerlist.o powers.o predicates.o stringutil.o timer.o unparse.o utils.o wild.o config.o $(COMM) $(DB) $(IO) $(PROG) $(IDENT) -lc -lm -lcrypt

