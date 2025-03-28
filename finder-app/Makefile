# Rule to build the writer executable
writer: writer.o
	$(CROSS_COMPILE)gcc writer.o -o writer

# Rule to compile writer.c into writer.o
writer.o: writer.c
	$(CROSS_COMPILE)gcc -c writer.c -o writer.o

# Rule to clean up generated files
clean:
	rm -f writer writer.o
