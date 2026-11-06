#ifndef KENSHICOOP_BUILD_RETRY_H
#define KENSHICOOP_BUILD_RETRY_H

namespace coop { namespace sync {
inline bool buildFinite(float value) {
    // Coordinates/progress outside this generous world bound are invalid;
    // comparisons also reject both infinities and NaN on the VC2010 runtime.
    return value>=-100000000.0f && value<=100000000.0f;
}
inline bool validBuildState(float progress, unsigned int complete) {
    return buildFinite(progress) && progress>=0.0f && complete<=1;
}
// Failed engine operations remain pending across network drains. Bound engine
// calls (not elapsed time): an unloaded zone does not consume an attempt.
struct BuildRetry {
    unsigned int attempts;
    unsigned long lastMs;
    bool stopped;
    BuildRetry() : attempts(0), lastMs(0), stopped(false) {}
    void reset() { attempts=0; lastMs=0; stopped=false; }
    bool due(unsigned long now) const {
        if(stopped || attempts>=8) return false;
        unsigned long delay=1000UL << (attempts>4 ? 4 : attempts);
        return attempts==0 || now-lastMs>=delay;
    }
    void attempted(unsigned long now) { ++attempts; lastMs=now; }
    void stop() { stopped=true; }
};
inline bool buildProgressMatches(float actual, int completed, float wanted, bool wantComplete) {
    float delta=actual-wanted;
    return completed==(wantComplete?1:0) && delta<0.005f && delta>-0.005f;
}
} }
#endif
