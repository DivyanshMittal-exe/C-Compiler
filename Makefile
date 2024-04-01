cc: cc.cpp c.tab.cpp c.lex.cpp
	g++ c.tab.cpp c.lex.cpp cc.cpp -lm -ll -lfl -o $@ `llvm-config --cxxflags --ldflags --system-libs --libs core` -fexceptions

c.tab.cpp c.tab.hpp: c.y
	bison -o c.tab.cpp -d c.y

c.lex.cpp: c.l c.tab.hpp
	flex -o c.lex.cpp -l c.l

clean:
	rm -f c.tab.cpp c.tab.hpp c.lex.cpp cc c.output

run_llvm: clean cc
	./cc examples/test1.c --dump-ast
	./a.ll

run: clean cc
	./cc examples/test1.c --dump-ast

run2: clean cc
	./cc --dump-ast examples/test2.c

run3: clean cc
	./cc examples/hello_world.c --dump-ast

run4: clean cc
	./cc examples/test3.c --dump-ast

SOURCES := $(wildcard ../more_examples/*.c)

run_extra: clean cc
	@for file in $(SOURCES); do \
	   echo "\n\n\n"; \
	   echo "Running cc with $$file..."; \
	   ./cc $$file; \
   done



