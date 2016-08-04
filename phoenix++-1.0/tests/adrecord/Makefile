HOME = ../..

include $(HOME)/Defines.mk

LIBS += -L$(HOME)/$(LIB_DIR) -l$(PHOENIX)

OBJS = adrecord.o

PROGS = adrecord

.PHONY: default all clean

default: all

all: $(PROGS)

adrecord: $(OBJS) $(LIB_DEP)
	$(CXX) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@ -I$(HOME)/$(INC_DIR)

clean:
	rm -f $(PROGS) $(OBJS)
