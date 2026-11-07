#ifndef DOS_GAME_H
#define DOS_GAME_H
/* C89, 16-bit int / 32-bit long. No floating point or DOS extender. */
#define MAX_EDGES 512
#define BANDS 25
#define BAND_EDGES 256
#define FP 64
/* About 1.3 seconds at 72.8 physics substeps/second. */
#define PLUNGER_MAX_CHARGE 96
#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_LAUNCH 4
#define KEY_NUDGE 8 /* X: nudge upward */
#define KEY_NUDGE_LEFT 16 /* A: bump from left, ball goes right */
#define KEY_NUDGE_RIGHT 32 /* D: bump from right, ball goes left */
typedef struct { short x,y,ex,ey,tx,ty,length,score,boost; } Edge;
typedef struct { short x,y,w,h; unsigned char pixels[1024]; } Sprite;
/* Runtime-only collision AABBs, not part of SCP1 on-disk format. */
typedef struct { short left,right,top,bottom; } EdgeBounds;
typedef struct {
    short count,launch_x,launch_y,drain_y,flipper[12];
    Edge edges[MAX_EDGES];
    EdgeBounds bounds[MAX_EDGES];
    Sprite sprites[5];
    unsigned short band[BANDS][BAND_EDGES], band_count[BANDS];
} Table;
typedef struct {
    short x,y,vx,vy,balls,waiting,over,charge,previous,hit_delay,event,nudge_delay;
    unsigned char sweep[2];
    /* Set on ball feed, cleared once the ball enters the main playfield.
     * Needed to distinguish an unsuccessful shooter return from a drain. */
    unsigned char in_shooter;
    unsigned char kick_armed[2],kick_side,kick_delay,kick_grace;
    unsigned char shake_frames,tilted,tilt_warning;
    unsigned char nudge_return_ticks,nudge_decay_ticks;
    short nudge_return_x,nudge_return_y,nudge_heat;
    unsigned short plunger_rng;
    unsigned long score,steps,hits;
    Edge flips[2][3]; /* rest, raised, intermediate sweep */
} Game;
int table_load(Table *t, const char *path);
int table_index(Table *t);
void game_reset(Game *g, const Table *t);
void game_step(Game *g, const Table *t, unsigned keys);
unsigned long integer_sqrt(unsigned long n);
#endif
