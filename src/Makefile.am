AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = modemapp
modem_model = SIM7600
modemapp_SOURCES = main.c at-queue.c at-control.c at-handlers.c at-recv.c ppp_procedure.c sim7600x.c system.c unix_client.c
modemapp_CFLAGS = -D${modem_model} -DMODEM_MODEL=\"${modem_model}\" -DPPP -DUNIX_CLI -DRESOLVED
