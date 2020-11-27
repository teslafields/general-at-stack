AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = modemapp
modemapp_SOURCES = main.c at-queue.c at-control.c at-handlers.c at-recv.c ppp_procedure.c sim7100x.c system.c unix_client.c
modemapp_CFLAGS = -DSIM7100 -DPPP -DUNIX_CLI
