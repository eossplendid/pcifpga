#ty3012s project makefile

OBJECTS = main.o xmlHandler.o discManager.o backStage.o autoScan.o appReadPacket.o \
	ProcessLayerHandler.o debugConsole.o

CC = powerpc-linux-gcc

XFLAGS = -I /usr/local/libxml2/include/libxml2/ \
	-L /usr/local/libxml2/lib/ -lxml2
#XFLAGS = -I /root/lib/libxml2/include/libxml2/ \
#	-L /root/lib/libxml2/lib/ -lxml2

ty3012s: $(OBJECTS)
	$(CC) -o ty3012s $(OBJECTS) -lpthread $(XFLAGS)

#$(OBJECTS): xmlHandler.h 

main.o: main.c main.h xmlHandler.h discManager.h common.h
	$(CC) -c main.c -lpthread $(XFLAGS)

xmlHandler.o: xmlHandler.c xmlHandler.h common.h
	$(CC) -c xmlHandler.c $(XFLAGS) -lpthread

discManager.o: discManager.c xmlHandler.h backStage.h discManager.h
	$(CC) -c discManager.c -lpthread $(XFLAGS)

backStage.o: backStage.c backStage.h common.h discManager.h
	$(CC) -c backStage.c -lpthread

autoScan.o: autoScan.c backStage.h autoScan.h
	$(CC) -c autoScan.c

appReadPacket.o: appReadPacket.c common.h
	$(CC) -c appReadPacket.c

ProcessLayerHandler.o: ProcessLayerHandler.c ProcessLayerHandler.h
	$(CC) -c ProcessLayerHandler.c

debugConsole.o: debugConsole.c debugConsole.h
	$(CC) -c debugConsole.c
	
.PHONY: clean
clean:
	rm ty3012s $(OBJECTS)

