DIRS=common KV_Server 
all:
	for DIR in $(DIRS);  do \
	    echo "*****In $$DIR";\
		$$(cd $$DIR; $(MAKE)) ; \
	done

clean:
	for DIR in $(DIRS);  do \
	    echo "*****In $$DIR";\
		$$( cd $$DIR; $(MAKE) clean); \
	done
