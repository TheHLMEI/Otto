#
# This list order supports dependencies between modules
#
SUBDIRS := ottolib  \
           ottojil  \
	   ottomsp  \
	   ottorep  \
	   ottosend \
	   ottosysd \
           ottoxlat


all: $(SUBDIRS)

$(SUBDIRS): ; $(MAKE) -C $@ $(MAKECMDGOALS)

# build any target by forwarding to $(dirs) rule
% : $(SUBDIRS) ;

.PHONY: $(SUBDIRS) all

