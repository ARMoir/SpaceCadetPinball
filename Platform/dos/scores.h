#ifndef DOS_SCORES_H
#define DOS_SCORES_H
/* Original: high_score.cpp; persistent top five, single local DOS player.
 * Minimal C89, with disk I/O strictly outside the active game loop. */
#define SCORE_COUNT 5
typedef struct {
    char initials[4]; /* three printable uppercase letters + NUL */
    unsigned long score;
} ScoreEntry;
extern ScoreEntry scores_table[SCORE_COUNT];
void scores_reset(void);
int scores_load(const char *path);
int scores_save(const char *path);
int scores_rank(unsigned long score);
int scores_insert(unsigned long score,const char *initials);
#endif
