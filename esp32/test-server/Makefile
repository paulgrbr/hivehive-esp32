build:
	clang++ -g client-test.cpp -o bin/client-test -lcurl
run: build
	./bin/client-test

debug: build
	lldb ./bin/client-test
