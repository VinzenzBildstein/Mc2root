.EXPORT_ALL_VARIABLES:

.SUFFIXES:

.PHONY: clean all

# := is only evaluated once

SHELL 		= /bin/sh

LIB_DIR 	= $(HOME)/lib
BIN_DIR		= $(HOME)/bin

NAME 		= Mc2root

ROOTLIBS     	:= $(shell root-config --libs)
ROOTINC      	:= -I$(shell root-config --incdir)

INCLUDES        = -I.

LIBRARIES	= 

CC		= gcc
CXX   = g++
CPPFLAGS	= $(ROOTINC) $(INCLUDES) -fPIC
CXXFLAGS	= -std=c++11 -pedantic -Wall -g -O3

LDFLAGS		= -g -fPIC

LDLIBS 		= -L$(LIB_DIR) $(ROOTLIBS) $(addprefix -l,$(LIBRARIES))

# for root: add a $(NAME)Dictionary.o
#LOADLIBES = \
#	$(NAME)Dictionary.o

# -------------------- implicit rules --------------------
# n.o made from n.c by 		$(CC) -c $(CPPFLAGS) $(CFLAGS)
# n.o made from n.cc by 	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS)
# n made from n.o by 		$(CC) $(LDFLAGS) n.o $(LOADLIBES) $(LDLIBS)

# -------------------- rules --------------------

# for library: add $(LIB_DIR)/lib$(NAME).so
all:  $(NAME) 
	@echo Done

# -------------------- libraries --------------------

lib$(NAME).so: $(LOADLIBES)
	$(CXX) $(LDFLAGS) -shared -Wl,-soname,lib$(NAME).so -o lib$(NAME).so.1.0.1 $(LOADLIBES) -lc
#	mv $@.1.0.1 $(LIB_DIR)

$(LIB_DIR)/libCommandLineInterface.so:
	@cd $(COMMON_DIR); make $@

# -------------------- pattern rules --------------------
# this rule sets the name of the .cc file at the beginning of the line (easier to find)

%.o: %.cc %.hh
	$(CXX) $< -c $(CPPFLAGS) $(CXXFLAGS) -o $@

# -------------------- default rule for executables --------------------

%: %.cc $(LOADLIBES)
	$(CXX) $< $(CXXFLAGS) $(CPPFLAGS) $(LOADLIBES) $(LDLIBS) -o $@
#	mv $@ $(BIN_DIR)

# -------------------- Root stuff --------------------

DEPENDENCIES = \
	RootLinkDef.h

$(NAME)Dictionary.o: $(NAME)Dictionary.cc $(NAME)Dictionary.h
	 $(CXX) -fPIC $(CXXFLAGS) $(CPPFLAGS) -c $<

$(NAME)Dictionary.cc: $(DEPENDENCIES)
	 rm -f $(NAME)Dictionary.cc $(NAME)Dictionary.h; rootcint -f $@ -c $(CPPFLAGS) $(DEPENDENCIES)

# -------------------- tar ball --------------------

tar:
	@echo "creating zipped tar-ball ... "
	tar -chvzf $(NAME).tar.gz Makefile \
	*.hh *.cc #\
#	 RootLinkDef.h

# -------------------- clean --------------------

clean:
	rm  -f $(NAME) *.o $(NAME)Dictionary.cc $(NAME)Dictionary.h
