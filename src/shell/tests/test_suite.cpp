#ifdef __clang__
#pragma clang diagnostic ignored "-Wc++98-compat"
#endif

#define CATCH_CONFIG_RUNNER
#include <Catch2/catch.hpp>

int main(int argc, char* argv[]) { return Catch::Session().run(argc, argv); }
