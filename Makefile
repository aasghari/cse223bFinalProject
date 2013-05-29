DIRS=common KV_Server 
all:
	for DIR in $(DIRS);  do \
	    echo "*****In $$DIR";\
		RES=$$(cd $$DIR; $(MAKE)) ; \
	done

clean:
	for DIR in $(DIRS);  do \
	    echo "*****In $$DIR";\
		RES=$$( cd $$DIR; $(MAKE) clean); \
	done
