#ifndef DOS_GAME_H
#define DOS_GAME_H
/* C89, 16-bit int / 32-bit long. No floating point or DOS extender. */
#define MAX_EDGES 640
#define MAX_BUMPERS 8
#define MAX_SLINGS 2 /* closed source-projected triangular sling outlines */
#define BANDS 25
#define BAND_EDGES 256
#define FP 64
/* F-006: five-pixel active flipper tip forgiveness. The original 2D
 * projection leaves a larger center gap than a fast, 3D rotating paddle
 * can reliably cover. No help is applied when idle or tilted. */
#define FLIPPER_TIP_ASSIST_PX 5
/* F-008 universal anti-stuck: ~1.5 seconds of no positional progress,
 * followed by ~1-second recovery stages at 72.8 physics steps/second. */
#define UNSTICK_DETECT_TICKS 110
#define UNSTICK_RETRY_TICKS 73
#define UNSTICK_PROGRESS_PX 3
#define UNSTICK_ESCAPE_PX 10
/* About 1.3 seconds at 72.8 physics substeps/second. */
#define PLUNGER_MAX_CHARGE 96
#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_LAUNCH 4
#define KEY_NUDGE 8 /* X: nudge upward */
#define KEY_NUDGE_LEFT 16 /* A: bump from left, ball goes right */
#define KEY_NUDGE_RIGHT 32 /* D: bump from right, ball goes left */
/* Standard DOS INT 33h: bit0=left, bit1=right, bit2=middle.
 * Optional third button matches the original options.cpp plunger bind. */
#define MOUSE_FLIPPER_KEYS(buttons) \
    ((((buttons)&1U)?KEY_LEFT:0U) | (((buttons)&2U)?KEY_RIGHT:0U))
#define MOUSE_GAME_KEYS(buttons) \
    (MOUSE_FLIPPER_KEYS(buttons) | (((buttons)&4U)?KEY_LAUNCH:0U))
typedef struct { short x,y,ex,ey,tx,ty,length,score,boost; } Edge;
typedef struct { short x,y,w,h; unsigned char pixels[1024]; } Sprite;
/* Runtime-only collision AABBs, not part of SCP2 on-disk format. */
typedef struct { short left,right,top,bottom; } EdgeBounds;
/* Source bumper caps are circular. SCP2 stores their eight outline edges;
 * derive a low-cost radial hit area once when the table is indexed. */
typedef struct { short first,x,y,radius,score,boost; } Bumper;
/* F-002: solid triangular slingshot cores, discovered from SCP2 closed
 * source outlines at load time. Normal vectors are Q8 outward normals.
 * Per-frame testing is just AABB rejection plus three dot products nearby. */
typedef struct {
    short first,count,left,right,top,bottom;
    short x[3],y[3],nx[3],ny[3];
} Sling;
typedef struct {
    short count,launch_x,launch_y,drain_y,flipper[12];
    short kickback[4]; /* Source-projected left/right strike centers. */
    Edge edges[MAX_EDGES];
    EdgeBounds bounds[MAX_EDGES];
    Bumper bumpers[MAX_BUMPERS];
    Sling slings[MAX_SLINGS];
    short sling_count;
    short bumper_count,outer_right_rail,outer_left_rail; /* exterior source rails */
    Sprite sprites[5];
    Sprite plunger[8];
    unsigned short band[BANDS][BAND_EDGES], band_count[BANDS];
} Table;
typedef struct {
    short x,y,vx,vy,balls,waiting,over,charge,previous,hit_delay,event,nudge_delay;
    unsigned char sweep[2];
    /* Set on ball feed, cleared once the ball enters the main playfield.
     * Needed to distinguish an unsuccessful shooter return from a drain. */
    unsigned char in_shooter;
    unsigned char kick_armed[2],kick_side,kick_delay,kick_flight;
    unsigned char event_bumper; /* 1-based cap index for audiovisual cues */
    unsigned char shake_frames,tilted,tilt_warning;
    unsigned char nudge_return_ticks,nudge_decay_ticks;
    short nudge_return_x,nudge_return_y,nudge_heat;
    unsigned short plunger_rng;
    unsigned long score,steps,hits;
    unsigned short nudges,kickbacks,tilts,boundary_hits,right_rail_saves,left_rail_saves;
    unsigned short flipper_contacts[2],flipper_power_contacts[2];
    unsigned short wall_crossings; /* F-002 swept static wall corrections */
    unsigned short sling_rescues[2]; /* F-002: closed left/right sling ejects */
    /* F-008: tiny fixed-point watchdog; no per-frame allocations. */
    short stuck_x,stuck_y,stuck_minx,stuck_maxx,stuck_miny,stuck_maxy;
    unsigned short stuck_ticks,stuck_stage_ticks;
    unsigned char stuck_stage,stuck_progress;
    unsigned short unstick_nudges,unstick_moves,unstick_relaunches;
    Edge flips[2][3]; /* rest, raised, intermediate sweep */
    Edge tip_assist[2]; /* only active on descending balls with held flipper */
} Game;
int table_load(Table *t, const char *path);
int table_index(Table *t);
void game_reset(Game *g, const Table *t);
void game_step(Game *g, const Table *t, unsigned keys);
unsigned long integer_sqrt(unsigned long n);
#endif
