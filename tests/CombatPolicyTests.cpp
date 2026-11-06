#include "../src/plugin/sync/CombatPolicy.h"
#include <cstdio>
#include <limits>
#define CHECK(x) do { if(!(x)) { std::printf("FAIL line %d: %s\n",__LINE__,#x); return 1; } } while(0)
using namespace coop;
using namespace coop::sync;
int main() {
    const u32 HOST=0, J1=1, J2=2;
    // Join -> host (original protocol-45 path) still works.
    CHECK(combatAddressed(J1,HOST,HOST));
    // local2: host -> join when the join authors the NPC.
    CHECK(combatAddressed(HOST,J1,J1));
    // Join -> join (relayed by the host) is addressed to the author only.
    CHECK(combatAddressed(J1,J2,J2));
    CHECK(!combatAddressed(J1,J2,HOST));
    // Self-addressed or out-of-range ids are rejected.
    CHECK(!combatAddressed(HOST,HOST,HOST));
    CHECK(!combatAddressed(MAX_PLAYERS,J1,J1));
    CHECK(!combatAddressed(J1,MAX_PLAYERS,J1));
    // Apply only while we still author the NPC and it is not a player body.
    CHECK(combatMayApply(J1,J1,J1,false));
    CHECK(!combatMayApply(J1,J1,HOST,false)); // authority moved back to host
    CHECK(!combatMayApply(J1,J1,J1,true));    // PvP is out of scope
    // Damage bounds reject zero, negative, NaN and infinity.
    CHECK(combatDamageValid(3.0f,0.0f));
    CHECK(combatDamageValid(0.0f,1.5f));
    CHECK(!combatDamageValid(0.0f,0.0f));
    CHECK(!combatDamageValid(-1.0f,5.0f));
    CHECK(!combatDamageValid(std::numeric_limits<float>::quiet_NaN(),1.0f));
    CHECK(!combatDamageValid(std::numeric_limits<float>::infinity(),1.0f));
    // Protocol 68: knockout-only reports (stealth assassination) are valid.
    CHECK(combatReportValid(0.0f,0.0f,30.0f));
    CHECK(combatReportValid(5.0f,1.0f,0.0f));
    CHECK(!combatReportValid(0.0f,0.0f,0.0f));
    CHECK(!combatReportValid(0.0f,0.0f,601.0f));
    CHECK(!combatReportValid(0.0f,0.0f,-1.0f));
    CHECK(!combatReportValid(0.0f,0.0f,std::numeric_limits<float>::quiet_NaN()));
    // Replay guard is per sender, monotonic, wrap-safe.
    CombatReplay r;
    CHECK(r.accept(HOST,1)); CHECK(!r.accept(HOST,1)); CHECK(r.accept(HOST,2));
    CHECK(r.accept(J1,1));   // independent counter per sender
    CHECK(!r.accept(J1,0));  // id 0 is never issued
    CHECK(r.accept(J2,0xfffffffeu)); CHECK(r.accept(J2,3)); // wrap
    r.forget(HOST); CHECK(r.accept(HOST,1)); // reconnect restarts the counter
    std::printf("PASS combat hit addressing (host<->join, join->join), authority, replay\n");
    return 0;
}
