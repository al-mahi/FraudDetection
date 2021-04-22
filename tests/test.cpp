#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

TEST_CASE("Testing basics"){
    int x = 1;
    int y = 2;
    REQUIRE(x+y == 3);
}



