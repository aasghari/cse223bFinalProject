DIRS=common KV_Server Web WebServer 
all:
	for DIR in $(DIRS);  do \
	    echo "*****In $$DIR";\
		`cd $$DIR; make`; \
	done


clean:
	for DIR in $(DIRS);  do \
	    echo "*****In $$DIR";\
		`cd $$DIR; make clean`; \
	done
