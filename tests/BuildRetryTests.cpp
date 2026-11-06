#include "../src/plugin/sync/BuildRetry.h"
#include <cstdio>
#include <limits>
#define CHECK(x) do { if(!(x)) { std::printf("FAIL line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main() {
    coop::sync::BuildRetry mint;
    CHECK(mint.due(0));
    mint.attempted(100);
    CHECK(!mint.due(2099)); CHECK(mint.due(2100));
    mint.attempted(2100);
    CHECK(!mint.due(6099)); CHECK(mint.due(6100));
    // Deferred/unloaded ticks never call attempted and preserve the budget.
    CHECK(mint.attempts==2);
    unsigned long now=6100;
    while(mint.attempts<8) { CHECK(mint.due(now)); mint.attempted(now); now+=32000; }
    CHECK(!mint.due(now)); CHECK(!mint.due(now+100000));
    // Faulted creation can have produced an object; it must not be repeated.
    mint.reset(); mint.stop(); CHECK(!mint.due(now));
    // A new state packet reopens its own write retry without reopening mint.
    coop::sync::BuildRetry state;
    state.attempted(now); CHECK(!state.due(now)); state.reset(); CHECK(state.due(now));
    CHECK(!mint.due(now));
    // Clock wrapping is subtraction-based, not an absolute deadline.
    state.attempted(0xfffffff0UL); CHECK(state.due(0x1000UL));
    // Construction progress is an engine total, not a [0,1] fraction.
    CHECK(coop::sync::buildProgressMatches(12.0f,0,12.001f,false));
    CHECK(!coop::sync::buildProgressMatches(12.0f,0,12.0f,true));
    CHECK(!coop::sync::buildProgressMatches(1.0f,0,2.0f,false));
    CHECK(!coop::sync::buildProgressMatches(std::numeric_limits<float>::quiet_NaN(),0,1.0f,false));
    CHECK(coop::sync::validBuildState(12.0f,1));
    CHECK(!coop::sync::validBuildState(-1.0f,0));
    CHECK(!coop::sync::validBuildState(12.0f,2));
    CHECK(!coop::sync::validBuildState(std::numeric_limits<float>::infinity(),0));
    std::printf("PASS building retry budget, timing, completion observation\n");
    return 0;
}
