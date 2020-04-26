OBJS := timer.o
LIBS=-lgpiod
CFLAGS=-D CONSUMER=\"$(PROJ)\"
timer: $(OBJS)
	$(CC) -o main $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)
$(OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LIBS)
