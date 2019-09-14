//
//  appTests.cpp
//  final-project-medhapatil
//
//  Created by Medha Patil on 5/1/19.
//

#define CATCH_CONFIG_RUNNER
#include "catch2.hpp"
#include "ofApp.h"

int main(int argc, char* argv[]) {
    int result = Catch::Session().run(1, argv);
    return result;
}

//To test if points on X-axis that are supposed to be year points are accurately calculated
TEST_CASE("GetDateAsFloat() method check") {
    int year = 1999;
    int month = 10;
    int day = 16;
    float date_as_float = ofApp::GetDateAsFloat(year, month, day);
    REQUIRE(date_as_float == 1999.875f);
}
