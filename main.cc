#include "stacktrace/stacktrace.h"
#include <iostream>

void baz() {
    std::cout << experimental::CurrentStackTrace() << std::endl;
}

void bar() {
    baz();
}

void foo() {
    bar();
}

int main() {
    foo();
}