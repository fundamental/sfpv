CXX := clang++
LLVMCOMPONENTS := cppbackend
RTTIFLAG := -fno-rtti
#RTTIFLAG :=
CXXFLAGS := $(shell llvm-config --cxxflags) $(RTTIFLAG) -g
LLVMLDFLAGS := $(shell llvm-config --ldflags --libs $(LLVMCOMPONENTS))
EXEC = name
CLANGLIBS = \
    -lclangFrontend \
    -lclangDriver \
    -lclangSerialization \
    -lclangParse \
    -lclangSema \
    -lclangAnalysis \
    -lclangRewrite \
    -lclangEdit \
    -lclangAST \
    -lclangLex \
    -lclangBasic \
    -lLLVMMC \
    -lLLVMSupport

all: main
	
main: main.o TranslationUnit.o Types.o GraphBuilder.o Errors.o
	$(CXX) -o main main.o TranslationUnit.o Types.o GraphBuilder.o Errors.o $(CLANGLIBS) $(LLVMLDFLAGS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

TranslationUnit.o: TranslationUnit.cpp
	$(CXX) $(CXXFLAGS) -c TranslationUnit.cpp

Types.o: Types.cpp
	$(CXX) $(CXXFLAGS) -c Types.cpp

GraphBuilder.o: GraphBuilder.cpp
	$(CXX) $(CXXFLAGS) -c GraphBuilder.cpp

Errors.o: Errors.cpp
	$(CXX) $(CXXFLAGS) -c Errors.cpp

clean:
	-rm -f main *.o *~
