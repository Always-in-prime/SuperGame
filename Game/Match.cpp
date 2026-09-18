#include "Match.h"
#include "World.h"

void Match_Init(Match& m) {
    m.roundEnding = false;
    m.roundEndTimer = 0.0f;
    m.lastWinner = -1;
    m.scoreLimit = 10;   // значение по умолчанию, World_Init перезапишет
}

bool Match_Update(Match& m, World& w, float dt) {
    (void)w;
    if (!m.roundEnding) return false;

    m.roundEndTimer -= dt;
    if (m.roundEndTimer <= 0.0f) {
        m.roundEnding = false;
        m.roundEndTimer = 0.0f;
        m.lastWinner = -1;
        return true;
    }
    return false;
}

void Match_EndRound(Match& m, int winnerIndex) {
    m.roundEnding = true;
    m.roundEndTimer = ROUND_END_DELAY;
    m.lastWinner = winnerIndex;
}

int Match_CheckWin(const Match& m, const World& w) {
    if (m.scoreLimit <= 0) return -1;
    if (w.players[0].score >= m.scoreLimit) return 0;
    if (w.players[1].score >= m.scoreLimit) return 1;
    return -1;
}