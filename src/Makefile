#
# This list order supports dependencies between modules
#
SUBDIRS := libotto   \
           ottocmd   \
           ottoexp   \
           ottoimp   \
           ottorep   \
           ottosysd  \
           ottotr


all: $(SUBDIRS)

$(SUBDIRS): ; $(MAKE) -C $@ $(MAKECMDGOALS)

# build any target by forwarding to $(dirs) rule
% : $(SUBDIRS) ;

.PHONY: $(SUBDIRS) all

