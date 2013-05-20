DIRS=KV_Server Web WebServer
all:
	for DIR in $(DIRS);  do \
		`cd $$DIR; make`; \
	done


clean:
	for DIR in $(DIRS);  do \
		`cd $$DIR; make` clean; \
	done
