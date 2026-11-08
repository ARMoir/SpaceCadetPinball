#include "game.h"
#include "scores.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static Table t;
static Game g;

static void fixture(void)
{
    const short flippers[12]={80,160,105,172,105,150,150,160,125,172,125,150};
    memset(&t,0,sizeof(t));
    t.count=1; t.launch_x=200; t.launch_y=150; t.drain_y=190;
    memcpy(t.flipper,flippers,sizeof(flippers));
    t.kickback[0]=55; t.kickback[1]=166;
    t.kickback[2]=175; t.kickback[3]=166;
    t.edges[0].x=100; t.edges[0].ex=100;
    t.edges[0].y=20; t.edges[0].ey=120;
    t.edges[0].ty=256; t.edges[0].length=100*FP;
    assert(table_index(&t)); game_reset(&g,&t);
}

/* Return true if the ball still overlaps a filled slingshot core.
 * Mirrors the source-derived Q8 half-plane test, independently of the
 * original disconnected line-segment collision index. */
static int overlaps_sling(const Game *ball,const Sling *h)
{
    int k;
    if(!h->count || ball->x<(h->left-3L)*FP ||
       ball->x>(h->right+3L)*FP || ball->y<(h->top-3L)*FP ||
       ball->y>(h->bottom+3L)*FP) return 0;
    for(k=0;k<3;k++) {
        long dx=(long)ball->x-(long)h->x[k]*FP;
        long dy=(long)ball->y-(long)h->y[k]*FP;
        long d=(dx*h->nx[k]+dy*h->ny[k])/256;
        if(d>=3*FP) return 0;
    }
    return 1;
}

/* F-002: a center can cross a thin wall between sampled positions.
 * Test both sides and both axis directions at legal playfield speeds. */
static void wall_guard_tests(void)
{
    int axis,side,speed;
    for(axis=0;axis<2;axis++) for(side=-1;side<=1;side+=2)
        for(speed=64;speed<=160;speed+=32) {
            fixture();
            if(axis) {
                t.edges[0].x=60; t.edges[0].ex=140;
                t.edges[0].y=t.edges[0].ey=100;
                t.edges[0].tx=256; t.edges[0].ty=0;
                t.edges[0].length=80*FP;
                assert(table_index(&t));
            }
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0; g.steps=1;
            g.x=axis?100*FP:100*FP+side*32;
            g.y=axis?100*FP+side*32:80*FP;
            g.vx=axis?0:-side*speed; g.vy=axis?-side*speed:0;
            game_step(&g,&t,0);
            assert(side*((axis?g.y:g.x)-100*FP)>=95);
            assert(side*(axis?g.vy:g.vx)>0);
            assert(g.balls==3 && g.score==0);
        }
    /* Diagonal wall, opposite approaches: no axis-only special case. */
    for(side=-1;side<=1;side+=2) {
        fixture(); t.edges[0].x=60; t.edges[0].y=60;
        t.edges[0].ex=140; t.edges[0].ey=140;
        t.edges[0].tx=t.edges[0].ty=181;
        t.edges[0].length=7241;
        assert(table_index(&t)); game_reset(&g,&t);
        g.waiting=0; g.in_shooter=0; g.steps=1;
        g.x=100*FP; g.y=100*FP+side*32;
        g.vx=side*128; g.vy=-side*128;
        game_step(&g,&t,0);
        assert(side*((long)g.y-g.x)>=130);
        assert(side*(g.vy-g.vx)>0 && g.wall_crossings==1);
    }
    /* Passing beyond an endpoint or moving away must not create a wall. */
    fixture(); g.waiting=0; g.in_shooter=0; g.steps=1;
    g.x=99*FP; g.y=130*FP; g.vx=160;
    game_step(&g,&t,0); assert(g.x>100*FP && g.vx==160);
    fixture(); g.waiting=0; g.in_shooter=0; g.steps=1;
    g.x=98*FP; g.y=80*FP; g.vx=-160;
    game_step(&g,&t,0); assert(g.x<98*FP && g.vx==-160);
    puts("STAGE: swept wall guards, endpoint passage and retreat passed");
    fflush(stdout);
}

int main(int argc,char **argv)
{
    int i,weak_vy,full_vy;
    FILE *f;
    wall_guard_tests();
    fixture();
    /* INT 33h button bits feed the existing flipper controls; gameplay
     * remains exactly the same for keyboard or mouse source. */
    assert(MOUSE_FLIPPER_KEYS(0)==0);
    assert(MOUSE_FLIPPER_KEYS(1)==KEY_LEFT);
    assert(MOUSE_FLIPPER_KEYS(2)==KEY_RIGHT);
    assert(MOUSE_FLIPPER_KEYS(3)==(KEY_LEFT|KEY_RIGHT));
    assert(MOUSE_FLIPPER_KEYS(4)==0); /* No third-button flipper. */
    assert(MOUSE_GAME_KEYS(4)==KEY_LAUNCH);
    assert(MOUSE_GAME_KEYS(7)==(KEY_LEFT|KEY_RIGHT|KEY_LAUNCH));
    assert(MOUSE_GAME_KEYS(8)==0);
    /* High-score table: sort, reject ties at full capacity, validate,
     * persist five scores, and fail safely on malformed/truncated data. */
    scores_reset();
    assert(scores_rank(0)==-1 && scores_rank(10)==0);
    assert(scores_insert(1000UL,"ABC")==0);
    assert(scores_insert(500UL,"XYZ")==1);
    assert(scores_insert(1000UL,"DEF")==1);
    assert(scores_insert(200UL,"BAD!")==-1);
    assert(scores_table[0].score==1000UL &&
           !strcmp(scores_table[0].initials,"ABC"));
    assert(scores_table[1].score==1000UL &&
           !strcmp(scores_table[1].initials,"DEF"));
    assert(scores_table[2].score==500UL &&
           !strcmp(scores_table[2].initials,"XYZ"));
    assert(scores_insert(300UL,"AAA")==3);
    assert(scores_insert(100UL,"BBB")==4);
    assert(scores_rank(100UL)==-1);
    assert(scores_save("SCTEST.DAT"));
    scores_reset();
    assert(scores_load("SCTEST.DAT"));
    assert(scores_table[0].score==1000UL &&
           !strcmp(scores_table[1].initials,"DEF") &&
           scores_table[4].score==100UL);
    f=fopen("SCTEST.DAT","w"); assert(f);
    fputs("SC1\nABC 9999999999\n",f); fclose(f);
    assert(!scores_load("SCTEST.DAT"));
    assert(scores_table[0].score==0);
    remove("SCTEST.DAT");
    puts("STAGE: source high-score ranks, file roundtrip and corrupt input");
    fflush(stdout);
    fixture(); g.waiting=0; g.x=90*FP; g.y=163*FP; g.vy=30;
    game_step(&g,&t,MOUSE_FLIPPER_KEYS(1));
    assert(g.vy<0 && g.flipper_contacts[0]>0);
    fixture(); g.waiting=0; g.x=140*FP; g.y=163*FP; g.vy=30;
    game_step(&g,&t,MOUSE_FLIPPER_KEYS(2));
    assert(g.vy<0 && g.flipper_contacts[1]>0);
    puts("STAGE: mouse left/right and dual-button flipper mapping"); fflush(stdout);
    fixture();
    puts("STAGE: basic fixtures"); fflush(stdout);
    assert(integer_sqrt(0)==0 && integer_sqrt(1)==1);
    assert(integer_sqrt(65535UL)==255 && integer_sqrt(4294967295UL)==65535UL);
    assert(g.balls==3 && g.waiting);
    /* F-008: frozen or jittering ball must recover, but ordinary gameplay,
     * the shooter, near-flipper holds, and natural drains must not.
     * Force the center to the same point before each physics tick to
     * model overlapping wall corrections which erase actual travel. */
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=125*FP; g.y=70*FP; g.score=6400;
    for(i=0;i<UNSTICK_DETECT_TICKS;i++) {
        g.x=125*FP; g.y=70*FP; g.vx=g.vy=0;
        game_step(&g,&t,0);
        if(i<UNSTICK_DETECT_TICKS-1)
            assert(!g.unstick_nudges && !g.unstick_moves);
    }
    assert(g.stuck_stage==1 && g.unstick_nudges==1 &&
           g.unstick_moves==0 && g.vy<0 && g.balls==3);
    for(i=0;i<UNSTICK_RETRY_TICKS;i++) {
        g.x=125*FP; g.y=70*FP; g.vx=g.vy=0;
        game_step(&g,&t,0);
    }
    assert(g.stuck_stage==2 && g.unstick_moves==1 &&
           (g.x!=125*FP || g.y!=70*FP) && g.score==6400);
    for(i=0;i<UNSTICK_RETRY_TICKS;i++) {
        g.x=125*FP; g.y=70*FP; g.vx=g.vy=0;
        game_step(&g,&t,0);
    }
    assert(g.stuck_stage==3 && g.unstick_moves==2);
    for(i=0;i<UNSTICK_RETRY_TICKS;i++) {
        g.x=125*FP; g.y=70*FP; g.vx=g.vy=0;
        game_step(&g,&t,0);
    }
    assert(g.waiting && g.in_shooter && !g.over && g.balls==3 &&
           g.score==6400 && g.unstick_nudges==1 &&
           g.unstick_moves==2 && g.unstick_relaunches==1 &&
           g.stuck_stage==0 && g.x==t.launch_x*FP &&
           g.y==t.launch_y*FP);
    puts("STAGE: 110-step detect, upward nudge, two safe moves and free relaunch");
    fflush(stdout);
    /* The last remaining ball also has an unconditional free recovery,
     * never a lost turn or accidental GAME OVER. Force the final stage. */
    fixture(); g.waiting=0; g.in_shooter=0;
    g.balls=1; g.score=7777;
    g.x=125*FP; g.y=70*FP;
    g.stuck_x=g.x; g.stuck_y=g.y;
    g.stuck_stage=3; g.stuck_stage_ticks=UNSTICK_RETRY_TICKS-1;
    game_step(&g,&t,0);
    assert(g.waiting && g.in_shooter && !g.over &&
           g.balls==1 && g.score==7777 &&
           g.unstick_relaunches==1 && g.stuck_stage==0);
    /* Moving 10+ pixels consistently after the first nudge cancels
     * escalation, rather than unjustly refeeding a rescued live ball. */
    fixture(); g.waiting=0; g.in_shooter=0;
    for(i=0;i<UNSTICK_DETECT_TICKS;i++) {
        g.x=125*FP; g.y=70*FP; g.vx=g.vy=0;
        game_step(&g,&t,0);
    }
    assert(g.stuck_stage==1 && g.unstick_nudges==1);
    for(i=0;i<6;i++) {
        g.x=140*FP; g.y=70*FP; g.vx=g.vy=0;
        game_step(&g,&t,0);
    }
    assert(g.stuck_stage==0 && !g.unstick_moves &&
           !g.unstick_relaunches && g.balls==3);
    /* Non-zero velocity is not sufficient if a collision repeatedly
     * forces the ball back into the same two-pixel neighborhood. */
    fixture(); g.waiting=0; g.in_shooter=0;
    for(i=0;i<UNSTICK_DETECT_TICKS;i++) {
        g.x=(short)((125+(i&1))*FP); g.y=70*FP;
        g.vx=40; g.vy=0;
        game_step(&g,&t,0);
    }
    assert(g.stuck_stage==1 && g.unstick_nudges==1);
    /* Do not treat intentionally held flippers, the shooter, kickback,
     * or the lower drain as stuck balls. */
    fixture();
    for(i=0;i<UNSTICK_DETECT_TICKS+80;i++)
        game_step(&g,&t,0);
    assert(!g.unstick_nudges && !g.unstick_moves);
    fixture(); g.waiting=0; g.in_shooter=1;
    g.x=200*FP; g.y=55*FP;
    for(i=0;i<UNSTICK_DETECT_TICKS+8;i++) {
        g.x=200*FP; g.y=55*FP; g.vx=g.vy=0;
        game_step(&g,&t,0);
    }
    assert(!g.unstick_nudges && !g.unstick_moves);
    fixture(); g.waiting=0; g.in_shooter=0;
    for(i=0;i<UNSTICK_DETECT_TICKS+8;i++) {
        g.x=90*FP; g.y=160*FP; g.vx=g.vy=0;
        game_step(&g,&t,KEY_LEFT);
    }
    assert(!g.unstick_nudges && !g.unstick_moves);
    fixture(); g.waiting=0; g.in_shooter=0;
    g.kick_delay=9;
    for(i=0;i<8;i++) game_step(&g,&t,0);
    assert(!g.unstick_nudges);
    fixture(); g.waiting=0; g.in_shooter=0;
    for(i=0;i<UNSTICK_DETECT_TICKS+8;i++) {
        g.x=115*FP; g.y=(t.drain_y-6)*FP; g.vx=g.vy=0;
        game_step(&g,&t,0);
    }
    assert(!g.unstick_nudges && !g.unstick_relaunches);
    fixture();
    puts("STAGE: watchdog excludes intended hold/shooter/normal drain");
    fflush(stdout);
    /* The original visibly pulls back over time. Fixed-point DOS charge
     * should continue over roughly 1.3 seconds, then stop at full pullback. */
    for(i=0;i<48;i++) game_step(&g,&t,KEY_LAUNCH);
    assert(g.waiting && g.charge==48);
    for(i=0;i<64;i++) game_step(&g,&t,KEY_LAUNCH);
    assert(g.waiting && g.charge==PLUNGER_MAX_CHARGE);
    puts("STAGE: charged"); fflush(stdout);
    game_step(&g,&t,0);
    full_vy=g.vy;
    assert(!g.waiting && full_vy<=-128 && full_vy>=-129 &&
           g.event==2 && g.vx==0);
    puts("STAGE: launch released"); fflush(stdout);
    fixture();
    game_step(&g,&t,KEY_LAUNCH);
    game_step(&g,&t,0);
    weak_vy=g.vy;
    assert(!g.waiting && weak_vy<=-44 && weak_vy>=-49);
    assert(full_vy<weak_vy);
    /* Two seed values at the same pullback must differ slightly, never
     * by enough to overpower the amount selected by the player. */
    fixture(); g.charge=PLUNGER_MAX_CHARGE; g.plunger_rng=0;
    game_step(&g,&t,KEY_LAUNCH); game_step(&g,&t,0);
    weak_vy=g.vy;
    fixture(); g.charge=PLUNGER_MAX_CHARGE; g.plunger_rng=1;
    game_step(&g,&t,KEY_LAUNCH); game_step(&g,&t,0);
    assert(g.vy!=weak_vy && abs(g.vy-weak_vy)<=6);
    fixture(); g.waiting=0; g.x=97*FP; g.y=60*FP; g.vx=128;
    game_step(&g,&t,0); assert(g.vx<0 && g.x<100*FP); puts("STAGE: wall bounce"); fflush(stdout);
    fixture(); t.edges[0].score=500; t.edges[0].boost=35;
    g.waiting=0; g.x=97*FP; g.y=60*FP; g.vx=128;
    game_step(&g,&t,0); assert(g.score==500 && g.hits==1);
    game_step(&g,&t,0); assert(g.score==500); puts("STAGE: scoring"); fflush(stdout);
    fixture(); g.waiting=0; g.x=90*FP; g.y=163*FP; g.vy=30;
    game_step(&g,&t,KEY_LEFT); assert(g.vy<0); puts("STAGE: left"); fflush(stdout);
    fixture(); g.waiting=0; g.x=140*FP; g.y=163*FP; g.vy=30;
    game_step(&g,&t,KEY_RIGHT); assert(g.vy<0); puts("STAGE: right"); fflush(stdout);
    /* The ball can sit between the resting and raised flipper lines.
     * The interpolated sweep must catch it during the key transition. */
    fixture(); g.waiting=0; g.x=103*FP; g.y=161*FP; g.vy=40;
    game_step(&g,&t,KEY_LEFT);
    assert(g.vy<0 && g.balls==3); puts("STAGE: midpoint"); fflush(stdout);
    /* F-006: favor player control over an unexpectedly large uncatchable
     * projected center gap. Only a held, raised tip reaches five pixels
     * farther into the middle; ordinary rest/drain physics stay intact.
     * Neither a pressless ball nor a tilted paddle can get this save. */
    for(i=0;i<2;i++) {
        int catch_x=i?120:110;
        unsigned key=i?KEY_RIGHT:KEY_LEFT;
        fixture(); g.waiting=0; g.in_shooter=0;
        g.x=catch_x*FP; g.y=148*FP; g.vy=50;
        game_step(&g,&t,0);
        assert(g.vy>0 && g.flipper_contacts[0]==0 &&
               g.flipper_contacts[1]==0 && g.balls==3);
        fixture(); g.waiting=0; g.in_shooter=0;
        g.x=catch_x*FP; g.y=148*FP; g.vy=50;
        game_step(&g,&t,key);
        assert(g.vy<0 && g.flipper_power_contacts[i]>0 &&
               g.balls==3);
        fixture(); g.waiting=0; g.in_shooter=0; g.tilted=1;
        g.x=catch_x*FP; g.y=148*FP; g.vy=50;
        game_step(&g,&t,key);
        assert(g.vy>0 && g.flipper_power_contacts[i]==0);
    }
    /* Even both activated flippers do not create a continuous invisible
     * bridge: a straight-down ball exactly in the center can still drain. */
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=115*FP; g.y=148*FP; g.vy=50;
    game_step(&g,&t,KEY_LEFT|KEY_RIGHT);
    assert(g.vy>0 && !g.flipper_contacts[0] &&
           !g.flipper_contacts[1]);
    /* Do not catch a ball already underneath the raised toe. */
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=110*FP; g.y=152*FP; g.vy=50;
    game_step(&g,&t,KEY_LEFT);
    assert(g.vy>0 && !g.flipper_power_contacts[0]);
    puts("STAGE: forgiving active center tips, idle drain and tilt"); fflush(stdout);
    /* A held raised flipper still collides on later steps. */
    fixture(); g.waiting=0; g.x=103*FP; g.y=149*FP; g.vy=40;
    game_step(&g,&t,KEY_LEFT);
    printf("RAISED: x=%d y=%d vx=%d vy=%d\n",g.x,g.y,g.vx,g.vy); fflush(stdout);
    assert(g.vx!=0 || g.vy!=40);
    puts("STAGE: raised"); fflush(stdout);
    /* F-006: original TFlipperEdge::EdgeCollision adds a leverage-based
     * front-face impulse to the incoming ball velocity. Test that impact
     * position changes power rather than launching the same fixed shot. */
    {
        short heel_vy,tip_vy,left_incoming_vx,right_incoming_vx;
        fixture(); g.waiting=0; g.in_shooter=0;
        g.x=84*FP; g.y=159*FP; g.vy=35;
        game_step(&g,&t,KEY_LEFT);
        heel_vy=g.vy;
        fixture(); g.waiting=0; g.in_shooter=0;
        g.x=100*FP; g.y=167*FP; g.vy=35;
        game_step(&g,&t,KEY_LEFT);
        tip_vy=g.vy;
        printf("FLIPPER LEVER heel=%d tip=%d\n",heel_vy,tip_vy);
        fflush(stdout);
        assert(heel_vy<0 && tip_vy<heel_vy-10);
        fixture(); g.waiting=0; g.in_shooter=0;
        g.x=90*FP; g.y=163*FP; g.vx=-24; g.vy=35;
        game_step(&g,&t,KEY_LEFT);
        left_incoming_vx=g.vx;
        fixture(); g.waiting=0; g.in_shooter=0;
        g.x=90*FP; g.y=163*FP; g.vx=24; g.vy=35;
        game_step(&g,&t,KEY_LEFT);
        right_incoming_vx=g.vx;
        assert(abs(right_incoming_vx-left_incoming_vx)>12);
        /* The mirrored right flipper must also add stronger power farther
         * from its pivot rather than only fixing the left-hand paddle. */
        fixture(); g.waiting=0; g.in_shooter=0;
        g.x=146*FP; g.y=159*FP; g.vy=35;
        game_step(&g,&t,KEY_RIGHT);
        heel_vy=g.vy;
        fixture(); g.waiting=0; g.in_shooter=0;
        g.x=130*FP; g.y=167*FP; g.vy=35;
        game_step(&g,&t,KEY_RIGHT);
        tip_vy=g.vy;
        printf("RIGHT LEVER heel=%d tip=%d\n",heel_vy,tip_vy);
        fflush(stdout);
        assert(heel_vy<0 && tip_vy<heel_vy-10);
        /* More powerful shots must still bounce from ordinary rails:
         * our four-pixel broad-phase margin covers <=2.75 px/step. */
        fixture(); g.waiting=0; g.in_shooter=0;
        g.x=97*FP; g.y=60*FP; g.vx=160;
        game_step(&g,&t,0);
        assert(g.vx<0 && g.x<100*FP);
        /* Launcher has its own 132 Q6 limit, while a moving flipper may
         * now drive the ball more strongly without being clipped to it. */
        fixture(); g.waiting=0; g.in_shooter=0;
        g.x=100*FP; g.y=167*FP; g.vy=35;
        game_step(&g,&t,KEY_LEFT);
        g.x=100*FP; g.y=145*FP;
        g.vy=-169; g.previous=0;
        game_step(&g,&t,0);
        assert(g.vy<=-160);
    }
    puts("STAGE: contact-sensitive flipper impulse"); fflush(stdout);
    /* Tilt cuts power, not the physical resting paddle. Check both sides
     * with the button held: contact must match an unpowered resting paddle,
     * with no sweep/launch impulse or score. */
    for(i=0;i<2;i++) {
        short rest_x,rest_y,rest_vx,rest_vy;
        fixture(); g.waiting=0; g.in_shooter=0;
        g.x=(i?140:90)*FP; g.y=163*FP; g.vy=30;
        game_step(&g,&t,0);
        rest_x=g.x; rest_y=g.y; rest_vx=g.vx; rest_vy=g.vy;
        assert(rest_vy<0);
        fixture(); g.waiting=0; g.in_shooter=0; g.tilted=1;
        g.x=(i?140:90)*FP; g.y=163*FP; g.vy=30;
        game_step(&g,&t,i?KEY_RIGHT:KEY_LEFT);
        assert(g.x==rest_x && g.y==rest_y &&
               g.vx==rest_vx && g.vy==rest_vy);
        assert(!g.sweep[i] && g.score==0);
    }
    puts("STAGE: tilted paddles retain passive collisions"); fflush(stdout);
    /* Directional source-like nudges add velocity and reverse it after
     * ~0.4s; key bounce is suppressed, but repeated bumps build tilt. */
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=50*FP; g.y=60*FP; g.vx=4; g.vy=10;
    game_step(&g,&t,KEY_NUDGE);
    assert(g.vy<0 && g.vx==4 && g.nudge_delay==9 &&
           g.nudges==1 && g.shake_frames);
    game_step(&g,&t,0); game_step(&g,&t,KEY_NUDGE);
    assert(g.nudges==1 && g.nudge_delay==7);
    for(i=0;i<30;i++) game_step(&g,&t,0);
    assert(g.nudge_return_ticks==0);
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=50*FP; g.y=60*FP;
    game_step(&g,&t,KEY_NUDGE_LEFT);
    assert(g.vx>0 && g.nudges==1);
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=50*FP; g.y=60*FP;
    game_step(&g,&t,KEY_NUDGE_RIGHT);
    assert(g.vx<0 && g.nudges==1);
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=50*FP; g.y=60*FP;
    for(i=0;i<3;i++) {
        int j;
        game_step(&g,&t,KEY_NUDGE_LEFT);
        game_step(&g,&t,0);
        for(j=0;j<9;j++) game_step(&g,&t,0);
    }
    assert(g.tilted && g.tilts==1 && g.nudges==3);
    puts("STAGE: source-like nudge and tilt passed"); fflush(stdout);
    /* Left kickback captures, winds up, fires into the table and disarms.
     * A second entry cannot trigger another save on the same ball. */
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=t.kickback[0]*FP; g.y=t.kickback[1]*FP; g.vy=32;
    game_step(&g,&t,0);
    assert(g.kick_delay==9 && g.kick_side==1 && !g.kick_armed[0] &&
           g.kick_armed[1] && g.kickbacks==1 && g.balls==3);
    for(i=0;i<9;i++) game_step(&g,&t,0);
    assert(g.kick_delay==0 && g.vx>0 && g.vy<0 &&
           g.event==4 && g.shake_frames>0 &&
           g.kick_gate_closed[0] && !g.kick_gate_closed[1]);
    g.x=t.kickback[0]*FP;
    g.y=t.kickback[1]*FP; g.vy=32;
    game_step(&g,&t,0);
    assert(!g.kick_delay && g.kickbacks==1);
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=t.kickback[2]*FP; g.y=t.kickback[3]*FP; g.vy=32;
    game_step(&g,&t,0);
    assert(g.kick_delay==9 && g.kick_side==2 && g.kickbacks==1);
    for(i=0;i<9;i++) game_step(&g,&t,0);
    assert(g.kick_delay==0 && g.vx<0 && g.vy<0 &&
           g.kick_gate_closed[1] && !g.kick_gate_closed[0]);
    puts("STAGE: both kickbacks passed"); fflush(stdout);
    /* Source TKickback's expired timer re-enables the matching TGate.
     * A new ball opens it under our simplified armed-at-start rule. */
    fixture(); g.waiting=0; g.in_shooter=0;
    g.kick_gate_closed[0]=g.kick_gate_closed[1]=1;
    g.kick_armed[0]=g.kick_armed[1]=0;
    g.x=80*FP; g.y=(t.drain_y+1)*FP; g.vy=64;
    game_step(&g,&t,0);
    assert(g.waiting && g.balls==2 &&
           !g.kick_gate_closed[0] && !g.kick_gate_closed[1] &&
           g.kick_armed[0] && g.kick_armed[1]);
    /* An open narrow gate must not reflect the ball; the same edge
     * becomes physically solid as soon as TGateEnable closes it. */
    for(i=0;i<2;i++) {
        int wallx=i?165:59;
        short vx=(short)(i?128:-128);
        fixture();
        t.edges[0].x=(short)wallx; t.edges[0].ex=(short)wallx;
        t.edges[0].y=119; t.edges[0].ey=139;
        t.edges[0].tx=0; t.edges[0].ty=256;
        t.edges[0].length=20*FP;
        assert(table_index(&t));
        t.gate_first[i]=0; /* isolated synthetic gate segment */
        game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
        g.x=(short)((i?wallx-2:wallx+2)*FP);
        g.y=130*FP; g.vx=vx; g.vy=0;
        game_step(&g,&t,0);
        assert((i?g.vx>0:g.vx<0) && !g.wall_crossings &&
               !g.kick_gate_closed[i]);
        game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
        g.kick_gate_closed[i]=1;
        g.x=(short)((i?wallx-2:wallx+2)*FP);
        g.y=130*FP; g.vx=vx; g.vy=0;
        game_step(&g,&t,0);
        assert((i?g.vx<0:g.vx>0) && g.kick_gate_closed[i] &&
               g.balls==3 && !g.waiting);
    }
    puts("STAGE: source kicker gate open/close and new-ball rearm");
    fflush(stdout);
    /* F-001: real SCP2 strike centers, including the right x=172.
     * A valid save MUST get to the upper quarter on an inward path,
     * then hand control back to standard collision and scoring. */
    for(i=0;i<2;i++) {
        int j;
        short last_y;
        fixture();
        t.kickback[0]=48; t.kickback[1]=155;
        t.kickback[2]=172; t.kickback[3]=155;
        g.waiting=0; g.in_shooter=0;
        g.x=t.kickback[i*2]*FP;
        g.y=t.kickback[i*2+1]*FP; g.vy=80;
        game_step(&g,&t,0);
        assert(g.kick_delay==9 && g.kick_side==i+1 &&
               !g.kick_flight && !g.kick_armed[i] && g.kickbacks==1);
        for(j=0;j<9;j++) game_step(&g,&t,0);
        assert(g.kick_flight && g.kick_delay==0 && g.event==4);
        last_y=g.y;
        for(j=0;j<64 && g.kick_flight;j++) {
            game_step(&g,&t,0);
            assert(g.y<last_y && g.vy==-160);
            assert(!g.waiting && !g.over && g.balls==3 &&
                   g.kickbacks==1 && g.score==0);
            last_y=g.y;
        }
        assert(!g.kick_flight && j>=35 && j<=60);
        assert(g.y<=48*FP && g.y>=45*FP);
        if(i==0) assert(g.x>80*FP && g.x<115*FP && g.vx>0);
        else assert(g.x>120*FP && g.x<155*FP && g.vx<0);
        /* Real walls are active immediately after the return. */
        g.x=97*FP; g.y=60*FP; g.vx=128; g.vy=0;
        game_step(&g,&t,0);
        assert(g.vx<0 && !g.kick_flight && g.balls==3);
        /* An already-consumed save never repeats on this ball. */
        g.x=t.kickback[i*2]*FP;
        g.y=t.kickback[i*2+1]*FP; g.vy=32;
        game_step(&g,&t,0);
        assert(!g.kick_delay && !g.kick_flight && g.kickbacks==1);
    }
    /* No invented protection for unarmed, climbing, tilted or shooter
     * contacts. Genuine outlane losses remain possible. */
    for(i=0;i<4;i++) {
        fixture(); g.waiting=0; g.in_shooter=0;
        g.x=t.kickback[2]*FP; g.y=t.kickback[3]*FP; g.vy=50;
        if(i==0) g.kick_armed[1]=0;
        if(i==1) g.vy=-50;
        if(i==2) g.tilted=1;
        if(i==3) g.in_shooter=1;
        game_step(&g,&t,0);
        assert(!g.kick_delay && !g.kick_flight && g.kickbacks==0);
    }
    puts("STAGE: both guided kickbacks reach upper table, no false saves");
    fflush(stdout);
    /* The original upper-left skill-shot passage is one-way. The
     * projected interior wall must let a descending ball through, but
     * still prevent it coming back upward through the same throat. */
    fixture();
    t.edges[0].x=94; t.edges[0].ex=104;
    t.edges[0].y=t.edges[0].ey=30;
    t.edges[0].tx=256; t.edges[0].ty=0;
    t.edges[0].length=10*FP;
    assert(table_index(&t));
    g.waiting=0; g.x=100*FP; g.y=29*FP; g.vy=100;
    game_step(&g,&t,0);
    assert(g.vy>0 && g.y>29*FP);
    fixture();
    t.edges[0].x=94; t.edges[0].ex=104;
    t.edges[0].y=t.edges[0].ey=30;
    t.edges[0].tx=256; t.edges[0].ty=0;
    t.edges[0].length=10*FP;
    assert(table_index(&t));
    g.waiting=0; g.x=100*FP; g.y=31*FP+60; g.vy=-45;
    game_step(&g,&t,0);
    assert(g.vy>=0);
    puts("STAGE: one-way chute"); fflush(stdout);
    /* F-002: a missing high ramp/rail group must not silently drain a
     * valid ball through the top or sides of the SCREEN. Only the true
     * lower playfield drain can consume a ball. A passive reflection
     * should keep these corner escape paths on the visible table. */
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=72*FP; g.y=-FP; g.vy=-100;
    game_step(&g,&t,0);
    assert(g.y==2*FP && g.vy>0 && g.balls==3 &&
           !g.waiting && g.boundary_hits==1);
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=-FP; g.y=40*FP; g.vx=-100;
    game_step(&g,&t,0);
    assert(g.x==2*FP && g.vx>0 && g.balls==3 &&
           !g.waiting && g.boundary_hits==1);
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=239*FP; g.y=50*FP; g.vx=100;
    game_step(&g,&t,0);
    assert(g.x==237*FP && g.vx<0 && g.balls==3 &&
           !g.waiting && g.boundary_hits==1);
    puts("STAGE: top/side safety rails preserve balls"); fflush(stdout);
    /* F-008: a traversing left/right slingshot loop is not stationary,
     * so the original 3px watchdog cannot detect it. Construct two
     * source-like convex Q8 triangles with mirror inward faces. Teleport
     * only between *test contacts*, never in game mechanics: this
     * deterministically drives genuine approach-side reflections.
     * Short volleys, same-side jitter and expired streaks stay normal. */
    {
        int side,j;
        static const short nx[3]={236,0,-236};
        static const short ny[3]={-95,256,-95};
        fixture();
        for(side=0;side<2;side++) {
            Sling *h=&t.slings[side];
            int center=side?150:70;
            memset(h,0,sizeof(*h));
            h->first=0; h->count=3;
            h->left=(short)(center-10);
            h->right=(short)(center+10);
            h->top=120; h->bottom=145;
            h->x[0]=(short)center;
            h->y[0]=120;
            h->x[1]=(short)(center+10);
            h->y[1]=145;
            h->x[2]=(short)(center-10);
            h->y[2]=145;
            for(j=0;j<3;j++) {
                h->nx[j]=nx[j];
                h->ny[j]=ny[j];
            }
        }
        t.sling_count=2;
        /* Back faces must remain solid but passive, without creating
         * an active sling hit, a bounce or a phantom anti-loop count.
         * Inner/front faces still rebound and count genuine strikes. */
        for(side=0;side<2;side++) {
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
            g.x=(short)((side?155:65)*FP);
            g.y=132*FP; g.vx=(short)(side?-96:96); g.vy=0;
            game_step(&g,&t,0);
            assert(!overlaps_sling(&g,&t.slings[side]) &&
                   g.sling_rescues[side]>0 &&
                   !g.sling_chain_hits && !g.sling_loop_breaks);
            assert(side?g.vx<=0:g.vx>=0);
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
            g.x=(short)((side?145:75)*FP);
            g.y=132*FP; g.vx=(short)(side?96:-96); g.vy=0;
            game_step(&g,&t,0);
            assert(!overlaps_sling(&g,&t.slings[side]) &&
                   g.sling_rescues[side]>0 &&
                   g.sling_chain_hits==1 &&
                   (side?g.vx<0:g.vx>0));
        }
        game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
        /* The first three alternating impacts are legitimate pinball. */
        for(j=0;j<3;j++) {
            side=j&1;
            g.x=(short)((side?144:76)*FP);
            g.y=135*FP;
            g.vx=(short)(side?96:-96); g.vy=0;
            game_step(&g,&t,0);
            assert(g.sling_rescues[side]>0 &&
                   g.sling_chain_hits==j+1 &&
                   g.sling_loop_breaks==0 &&
                   g.balls==3 && !g.waiting);
        }
        /* The fourth true reversal escapes upward, not down the drain. */
        g.x=144*FP; g.y=135*FP; g.vx=96; g.vy=0;
        game_step(&g,&t,0);
        assert(g.sling_loop_breaks==1 &&
               !g.sling_chain_hits && !g.sling_chain_time &&
               g.vx==-24 && g.vy==-64 &&
               g.balls==3 && g.score==0 && !g.waiting);
        /* Repeatedly touching only one slingshot cannot trigger it. */
        game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
        for(j=0;j<12;j++) {
            g.x=76*FP; g.y=135*FP; g.vx=-96; g.vy=0;
            game_step(&g,&t,0);
        }
        assert(g.sling_loop_breaks==0 && g.sling_chain_hits==1);
        /* A missed two-second window discards the old streak. */
        g.sling_chain_time=1;
        g.x=110*FP; g.y=110*FP; g.vx=0; g.vy=0;
        game_step(&g,&t,0);
        assert(!g.sling_chain_hits && !g.sling_chain_side);
        /* Crossing above the slings also resets a partial volley. */
        g.sling_chain_hits=3; g.sling_chain_time=100;
        g.sling_chain_side=1;
        g.x=110*FP; g.y=90*FP; g.vx=0; g.vy=0;
        game_step(&g,&t,0);
        assert(!g.sling_chain_hits && !g.sling_chain_time);
        /* No rescue is granted to an active shooter or tilted ball. */
        g.sling_chain_hits=3; g.sling_chain_time=100;
        g.sling_chain_side=1; g.tilted=1;
        g.x=144*FP; g.y=135*FP; g.vx=96; g.vy=0;
        game_step(&g,&t,0);
        assert(!g.sling_loop_breaks && !g.sling_chain_hits);
        puts("STAGE: F-008 alternating-sling anti-loop guard, normal volleys and exclusions");
        fflush(stdout);
    }
    /* A failed launch may roll back past the shared drain height.
     * It must return to the plunger without consuming a ball. */
    fixture(); g.waiting=0; g.x=(t.launch_x+2)*FP;
    g.y=(t.drain_y+1)*FP; g.vy=32;
    game_step(&g,&t,0);
    assert(g.balls==3 && g.waiting && !g.over && g.event==0);
    assert(g.x==t.launch_x*FP && g.y==t.launch_y*FP);
    assert(g.vx==0 && g.vy==0 && g.charge==0);
    assert(g.in_shooter);
    /* The diagonal lower tube returns from well to the right of
     * launch_x; the old +/-8 check failed and drained this shot. */
    fixture(); g.waiting=0;
    g.x=(t.launch_x+18)*FP; g.y=(t.drain_y-12)*FP;
    g.vy=32;
    game_step(&g,&t,0);
    assert(g.waiting && g.balls==3 && g.in_shooter);
    /* A ball that reached the main playfield must drain normally. */
    fixture(); g.waiting=0;
    g.x=(t.launch_x-22)*FP; g.y=(t.launch_y-30)*FP;
    game_step(&g,&t,0);
    assert(!g.in_shooter);
    g.x=(t.launch_x+18)*FP; g.y=(t.drain_y+1)*FP;
    g.vy=32;
    game_step(&g,&t,0);
    assert(g.waiting && g.balls==2 && g.in_shooter);
    fixture(); g.waiting=0;
    g.x=(t.launch_x+2)*FP; g.y=(t.drain_y+1)*FP;
    g.vy=32;
    game_step(&g,&t,0);
    game_step(&g,&t,KEY_LAUNCH);
    game_step(&g,&t,0);
    assert(!g.waiting && g.vy<0 && g.event==2);
    /* The adjacent main playfield still drains normally. */
    fixture(); g.waiting=0; g.x=(t.launch_x-12)*FP;
    g.y=(t.drain_y+1)*FP; g.vy=32;
    game_step(&g,&t,0);
    assert(g.balls==2 && g.waiting && g.event==3);
    assert(g.kick_armed[0] && g.kick_armed[1] && !g.tilted);
    fixture();
    for(i=0;i<3;i++) {
        g.waiting=0; g.y=192*FP; g.x=110*FP;
        game_step(&g,&t,0); assert(g.balls==2-i);
    }
    assert(g.over);
    game_reset(&g,&t); assert(g.score==0 && !g.over && g.balls==3);
    /* Exercise the actual imported pack under 16-bit arithmetic. */
    puts("STAGE: synthetic tests passed; loading imported table"); fflush(stdout);
    assert(table_load(&t,"TABLE.DAT")); game_reset(&g,&t);
    puts("STAGE: imported pack loaded"); fflush(stdout);
    /* Original SCP2 has narrow four-edge v_gate1/v_gate2 source
     * groups inside the kicker lanes, not at invented coordinates. */
    assert(t.gate_first[0]>=0 && t.gate_first[1]>=0);
    assert(t.gate_left[0]>=55 && t.gate_right[0]<=72 &&
           t.gate_top[0]>=115 && t.gate_bottom[0]<=145);
    assert(t.gate_left[1]>=160 && t.gate_right[1]<=170 &&
           t.gate_top[1]>=115 && t.gate_bottom[1]<=145);
    printf("SOURCE GATES L:%d x%d..%d y%d..%d R:%d x%d..%d y%d..%d\n",
           t.gate_first[0],t.gate_left[0],t.gate_right[0],
           t.gate_top[0],t.gate_bottom[0],t.gate_first[1],
           t.gate_left[1],t.gate_right[1],
           t.gate_top[1],t.gate_bottom[1]);
    fflush(stdout);
    /* Redundant closed detail loops inside solid slingshot hulls
     * (SCP2 edges 446..453) must not create secondary rebounds. */
    for(i=0;i<BANDS;i++) {
        int k;
        for(k=0;k<t.band_count[i];k++) {
            int e=t.band[i][k];
            assert(e<446 || e>453);
        }
    }
    /* F-002: slingshots are irregular CLOSED source outlines, not
     * collections of independent two-sided wall capsules. Both must
     * index as solid faces, especially the problem right sling. */
    {
        int side,trial,step;
        static const short vx[7]={0,160,-160,0,0,135,-135};
        static const short vy[7]={0,0,0,160,-160,145,145};
        static const short ox[7]={0,-15,15,0,0,-13,13};
        static const short oy[7]={0,0,0,-11,13,-11,-11};
        assert(t.sling_count==2);
        for(side=0;side<2;side++) {
            const Sling *h=&t.slings[side];
            int cx=(h->x[0]+h->x[1]+h->x[2])/3;
            int cy=(h->y[0]+h->y[1]+h->y[2])/3;
            assert(h->count>=3 && h->count<=20 &&
                   h->right-h->left>=12 && h->right-h->left<=18 &&
                   h->bottom-h->top>=22 && h->bottom-h->top<=29);
            assert((side==0 && h->right<=t.flipper[0]+2) ||
                   (side==1 && h->left>=t.flipper[6]-2));
            /* No old two-sided sling edge may also run in the normal
             * indexed collision path and shove a ball back inside. */
            for(trial=0;trial<BANDS;trial++) {
                int z;
                for(z=0;z<t.band_count[trial];z++) {
                    int edge=t.band[trial][z];
                    assert(edge<h->first || edge>=h->first+h->count);
                }
            }
            for(trial=0;trial<7;trial++) {
                game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
                g.x=(short)((cx+ox[trial])*FP);
                g.y=(short)((cy+oy[trial])*FP);
                g.vx=vx[trial]; g.vy=vy[trial];
                if(!trial) assert(overlaps_sling(&g,h));
                for(step=0;step<48 && !g.over && !g.waiting;step++) {
                    game_step(&g,&t,0);
                    if(overlaps_sling(&g,h)) {
                        printf("SLING TRAP side=%d trial=%d step=%d x=%d y=%d\n",
                               side,trial,step,g.x/FP,g.y/FP);
                        fflush(stdout);
                    }
                    assert(!overlaps_sling(&g,h));
                }
                if(!trial) {
                    printf("SOLID SLING %c: x=%d y=%d rescues=%u\n",
                           side?'R':'L',g.x/FP,g.y/FP,
                           g.sling_rescues[side]);
                    fflush(stdout);
                    assert(g.sling_rescues[side]>0 && g.balls==3);
                }
            }
        }
        puts("STAGE: original slingshot centers are solid on both sides");
        fflush(stdout);
    }
    if(argc>1 && !strcmp(argv[1],"/original")) {
        int v,bumper_count=0;
        /* F-002: hardware FLIGHT log 2026-11-07 frames 1432-1455
         * shows an actual ball passing the original right exterior rail
         * (x ~= 181 at y=100) and reaching x=235 before the SCREEN
         * fallback kicked in. Require a source-derived right boundary,
         * not a fabricated speed cap or random kick. */
        {
            const Edge *rail;
            game_reset(&g,&t);
            assert(t.outer_right_rail>=0);
            rail=&t.edges[t.outer_right_rail];
            assert((rail->x>rail->ex?rail->x:rail->ex)>=190);
            g.waiting=0; g.in_shooter=0;
            g.x=185*FP; g.y=102*FP; g.vx=158; g.vy=33;
            game_step(&g,&t,0);
            printf("REAL RIGHT RAIL: x=%d y=%d vx=%d screen=%u rail=%u\\n",
                   g.x/FP,g.y/FP,g.vx,g.boundary_hits,g.right_rail_saves);
            fflush(stdout);
            assert(g.right_rail_saves==1 && g.x/FP<183 &&
                   g.vx<0 && g.boundary_hits==0 && g.balls==3);
            /* A ball safely inboard should not be affected by the guard. */
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
            g.x=153*FP; g.y=100*FP; g.vx=20; g.vy=0;
            game_step(&g,&t,0);
            assert(g.right_rail_saves==0 && g.boundary_hits==0);
        }
        /* Mirror the containment check against the SOURCE left exterior,
         * rather than allowing an invisible x=2 screen edge to rescue it. */
        {
            const Edge *rail;
            game_reset(&g,&t);
            assert(t.outer_left_rail>=0);
            rail=&t.edges[t.outer_left_rail];
            assert((rail->x<rail->ex?rail->x:rail->ex)<=42);
            g.waiting=0; g.in_shooter=0;
            g.x=20*FP; g.y=102*FP; g.vx=-145; g.vy=30;
            game_step(&g,&t,0);
            printf("REAL LEFT RAIL: x=%d y=%d vx=%d screen=%u rail=%u\n",
                   g.x/FP,g.y/FP,g.vx,g.boundary_hits,g.left_rail_saves);
            fflush(stdout);
            assert(g.left_rail_saves==1 && g.x/FP>=48 &&
                   g.vx>0 && g.boundary_hits==0 &&
                   g.right_rail_saves==0 && g.balls==3);
            /* Inboard shots should never be snapped to the rail. */
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
            g.x=112*FP; g.y=102*FP; g.vx=-10; g.vy=4;
            game_step(&g,&t,0);
            assert(g.left_rail_saves==0 && g.right_rail_saves==0);
            /* Crucially, a ball climbing the valid SHOOTER lane to the
             * right of the playfield must not hit our right rail guard. */
            game_reset(&g,&t); g.waiting=0; g.in_shooter=1;
            g.x=185*FP; g.y=102*FP; g.vx=0; g.vy=-50;
            game_step(&g,&t,0);
            assert(g.in_shooter && g.right_rail_saves==0 &&
                   g.left_rail_saves==0 && g.balls==3);
        }
        puts("STAGE: source left/right rail envelope and shooter passed"); fflush(stdout);
        /* Every visible source bumper must exist and respond. The last
         * three used to be filtered out as upper-layer geometry. Approach
         * each cap from its right edge on the complete imported table. */
        for(v=0;v<t.count;v++) if(t.edges[v].score) {
            int j,right=t.edges[v].x,top=t.edges[v].y,bottom=top;
            assert(v+7<t.count);
            for(j=0;j<8;j++) {
                const Edge *e=&t.edges[v+j];
                assert(e->score==t.edges[v].score);
                if(e->x>right) right=e->x;
                if(e->y<top) top=e->y;
                if(e->y>bottom) bottom=e->y;
            }
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
            g.x=(right+4)*FP; g.y=(top+bottom)*FP/2; g.vx=-64;
            for(j=0;j<20 && !g.hits;j++) game_step(&g,&t,0);
            assert(g.hits==1 && g.score==(unsigned)t.edges[v].score);
            bumper_count++; v+=7;
        }
        assert(bumper_count==7);
        assert(t.bumper_count==7);
        /* F-004: each closed scoring cap should use a radial normal,
         * not eight interior-facing two-sided capsule edges. A near-center
         * entry at the upper-left bumper must be pushed OUT of the cap
         * and rebound without staying trapped between polygon segments. */
        {
            const Bumper *cap=&t.bumpers[3];
            long dx,dy,dist2;
            assert(cap->x/FP>=72 && cap->x/FP<=81);
            assert(cap->y/FP>=17 && cap->y/FP<=25);
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
            g.x=(short)(cap->x+cap->radius/4);
            g.y=cap->y;
            g.vx=-80; g.vy=0;
            game_step(&g,&t,0);
            dx=(long)g.x-cap->x; dy=(long)g.y-cap->y;
            dist2=dx*dx+dy*dy;
            assert(dist2 >= (long)(cap->radius-32)*(cap->radius-32));
            assert(g.vx>0 && g.hits==1 && g.balls==3);
            /* Approaching from the right should bounce on the exterior
             * of the same source bumper, not reverse toward its center. */
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
            g.x=(short)(cap->x+cap->radius+FP/2);
            g.y=cap->y; g.vx=-100; g.vy=0;
            for(i=0;i<12 && !g.hits;i++) game_step(&g,&t,0);
            assert(g.hits==1 && g.vx>0);
        }
        puts("STAGE: radial upper-left bumper escape"); fflush(stdout);
        /* F-006: reproduce the user's actual /record flight near the
         * right flipper. The original-table flipper is at (140,158),
         * resting at (125,167), raised at (125,149). A descending ball
         * near (127,149) must meet a held raised flipper; one near
         * (127,164) must meet a released resting flipper. The old synthetic
         * fixture did not cover these actual source coordinates or
         * collisions against neighboring imported rails. */
        {
            int q,was_reflected;
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
            g.x=127*FP+32; g.y=147*FP;
            g.vx=-1; g.vy=11; g.previous=KEY_RIGHT;
            was_reflected=0;
            for(q=0;q<20;q++) {
                game_step(&g,&t,KEY_RIGHT);
                if(g.vy<0) { was_reflected=1; break; }
            }
            printf("REAL RIGHT HELD: reflected=%d x=%d y=%d vy=%d\n",
                   was_reflected,g.x/FP,g.y/FP,g.vy);
            fflush(stdout);
            assert(was_reflected && g.balls==3);
            /* A held, raised flipper must not give only the old weak
             * ~-21 Q6 passive bounce, even with no fresh key transition. */
            printf("REAL RIGHT HELD power=%u passive=%u\n",
                   g.flipper_power_contacts[1],g.flipper_contacts[1]);
            fflush(stdout);
            assert(g.flipper_power_contacts[1]>=1 && g.vy<=-35);
            /* A late fresh right-key press during the ball's actual
             * incoming y=147..150 path must carry full *moving* boost
             * all the way through the raised tip, rather than dropping
             * back to passive immediately after the intermediate edge. */
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
            g.x=127*FP+32; g.y=147*FP;
            g.vx=-1; g.vy=11; g.previous=0;
            was_reflected=0;
            for(q=0;q<20;q++) {
                game_step(&g,&t,KEY_RIGHT);
                if(g.vy<0) { was_reflected=1; break; }
            }
            printf("REAL RIGHT PRESS: reflected=%d x=%d y=%d vy=%d power=%u\n",
                   was_reflected,g.x/FP,g.y/FP,g.vy,
                   g.flipper_power_contacts[1]);
            fflush(stdout);
            assert(was_reflected && g.vy<=-60 &&
                   g.flipper_power_contacts[1]>=1);
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
            g.x=126*FP+45; g.y=162*FP;
            g.vx=-1; g.vy=31; g.previous=0;
            was_reflected=0;
            for(q=0;q<10;q++) {
                game_step(&g,&t,0);
                if(g.vy<0) { was_reflected=1; break; }
            }
            printf("REAL RIGHT REST: reflected=%d x=%d y=%d vy=%d\n",
                   was_reflected,g.x/FP,g.y/FP,g.vy);
            fflush(stdout);
            assert(was_reflected && g.balls==3);
            assert(g.flipper_power_contacts[1]==0);
        }
        puts("STAGE: real right-flipper playback contact"); fflush(stdout);
        /* F-006 source geometry: raised tips (107,149) and (125,149)
         * leave a center gap far larger than a 3px ball capsule.
         * The new held-only 5px toe assists must save both approaches
         * without changing the artwork or rescuing a released paddle. */
        for(v=0;v<2;v++) {
            int xx=v?120:112;
            unsigned key=v?KEY_RIGHT:KEY_LEFT;
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
            g.x=xx*FP; g.y=147*FP; g.vx=0; g.vy=50;
            g.previous=(short)key; /* held, no new sweep transition */
            game_step(&g,&t,key);
            printf("REAL CENTER TIP %c: vy=%d power=%u\n",
                   v?'R':'L',g.vy,g.flipper_power_contacts[v]);
            fflush(stdout);
            assert(g.vy<0 && g.flipper_power_contacts[v]>0 &&
                   g.balls==3);
            game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
            g.x=xx*FP; g.y=147*FP; g.vx=0; g.vy=50;
            game_step(&g,&t,0);
            assert(g.vy>0 && !g.flipper_contacts[v]);
        }
        puts("STAGE: source geometry forgiving center tips"); fflush(stdout);
        /* Strike from the imported anchors, including offset entries.
         * For the first return arc, both sides must remain on the table. */
        for(v=0;v<2;v++) {
            int offset;
            for(offset=-3;offset<=3;offset+=3) {
                int highest=t.kickback[v*2+1];
                game_reset(&g,&t); g.waiting=0; g.in_shooter=0;
                g.x=(t.kickback[v*2]+offset)*FP;
                g.y=t.kickback[v*2+1]*FP; g.vy=32;
                game_step(&g,&t,0);
                assert(g.kick_delay==9 && g.kickbacks==1);
                for(i=0;i<60;i++) {
                    game_step(&g,&t,0);
                    assert(g.x>=30*FP && g.x<=204*FP);
                    assert(g.y>=4*FP && g.y<t.drain_y*FP);
                    assert(g.balls==3 && !g.waiting);
                    if(g.y/FP<highest) highest=g.y/FP;
                }
                assert(highest<t.kickback[v*2+1]-20);
            }
        }
        /* Original plunger art belongs in the shooter, not outside the
         * right-hand frame. Rest and fully pulled frames have distinct art. */
        assert(t.plunger[0].x==184 && t.plunger[0].y==156);
        assert(t.plunger[7].x==185 && t.plunger[7].y==166);
        /* F-003: the projected launch anchor (181,141) leaves the
         * READY ball 15px above the original plunger sprite. Seat the
         * 4px ball with its bottom ~1px above the retracted rod tip.
         * Do not modify SCP2 geometry or shift an active ball sprite. */
        game_reset(&g,&t);
        assert(t.launch_x==181 && t.launch_y==141);
        assert(g.x==(t.plunger[0].x+t.plunger[0].w/2)*FP);
        assert(g.y==(t.plunger[0].y-3)*FP);
        assert(g.y/FP + t.sprites[4].h/2 - 1 < t.plunger[0].y &&
               t.plunger[0].y-(g.y/FP + t.sprites[4].h/2 - 1)<=3);
        assert(g.waiting && g.in_shooter && g.balls==3);
        /* Regression from physical Tandy: charging from the visually
         * seated ball must clear the lower tube AND enter the upper
         * playfield, not just register a score after many retries. */
        for(v=0;v<4;v++) {
            int j,upper=0;
            short last_x,last_y;
            game_reset(&g,&t); g.plunger_rng=(unsigned short)v;
            for(j=0;j<PLUNGER_MAX_CHARGE;j++)
                game_step(&g,&t,KEY_LAUNCH);
            game_step(&g,&t,0);
            assert(!g.waiting && g.in_shooter &&
                   g.shooter_seat_guide==SHOOTER_SEAT_GUIDE_TICKS &&
                   g.vy<=-128);
            last_x=g.x; last_y=g.y;
            for(j=0;j<SHOOTER_SEAT_GUIDE_TICKS;j++) {
                game_step(&g,&t,0);
                assert(g.x<=last_x && g.y<last_y &&
                       g.in_shooter && !g.waiting);
                last_x=g.x; last_y=g.y;
            }
            assert(!g.shooter_seat_guide &&
                   g.x==t.launch_x*FP && g.y==t.launch_y*FP);
            for(j=0;j<500 && g.in_shooter && !g.waiting;j++) {
                game_step(&g,&t,0);
                if(g.y<90*FP) upper=1;
            }
            printf("F003 FULL: seed=%d upper=%d exited=%d steps=%d xy=%d,%d\n",
                   v,upper,!g.in_shooter,j,g.x/FP,g.y/FP);
            fflush(stdout);
            assert(upper && !g.in_shooter && !g.waiting &&
                   !g.over && g.balls==3);
        }
        game_reset(&g,&t);
        for(v=0;v<8;v++) {
            assert(t.plunger[v].x>=t.launch_x-3 && t.plunger[v].x<195);
            assert(t.plunger[v].y+t.plunger[v].h<=192);
        }
        puts("STAGE: seven bumpers, confined kickbacks and source plunger passed"); fflush(stdout);
        /* Real trajectories, rather than teleporting a ball to the drain:
         * quarter-power shots turn back inside the curved right-hand tube.
         * They must return to READY with the same ball and allow relaunch. */
        for(v=24;v<=26;v++) {
            int seed;
            for(seed=0;seed<4;seed++) {
                game_reset(&g,&t); g.plunger_rng=(unsigned short)seed;
                for(i=0;i<v;i++) game_step(&g,&t,KEY_LAUNCH);
                game_step(&g,&t,0);
                assert(!g.waiting && g.vy<0);
                for(i=0;i<1500 && !g.waiting && !g.over;i++) {
                    game_step(&g,&t,0);
                    assert(g.in_shooter && g.balls==3);
                }
                assert(i<1500 && g.waiting && g.balls==3 && !g.over);
                assert(g.x==(t.plunger[0].x+t.plunger[0].w/2)*FP &&
                       g.y==(t.plunger[0].y-3)*FP);
                assert(g.vx==0 && g.vy==0 && g.charge==0 && g.hits==0);
                /* Retain the same ball and allow a second deliberate
                 * plunger pullback with a *different* charge strength. */
                for(i=0;i<48;i++) game_step(&g,&t,KEY_LAUNCH);
                assert(g.charge==48 && g.waiting && g.balls==3);
                game_step(&g,&t,0);
                assert(!g.waiting && g.vy<-70 && g.event==2 &&
                       g.balls==3);
            }
        }
        puts("STAGE: partial-charge shooter-return trajectories passed"); fflush(stdout);
        /* Full-charge variation must not strand the ball in the shooter.
         * Check the possible 128..129 Q6 launch impulses after charging:
         * unlike phase-zero tests, release occurs at substep 97. */
        for(v=128;v<=129;v++) {
            game_reset(&g,&t);
            g.waiting=0; g.vx=0; g.vy=(short)-v;
            g.steps=97; g.shooter_seat_guide=SHOOTER_SEAT_GUIDE_TICKS;
            for(i=0;i<2500;i++) {
                unsigned keys=(i+97)%80<40?KEY_LEFT:KEY_RIGHT;
                game_step(&g,&t,keys);
                if(g.waiting) break;
            }
            printf("POWER %d: hits=%lu score=%lu steps=%d\n",
                   v,g.hits,g.score,i);
            fflush(stdout);
            assert(g.hits>0 && g.score>=500);
        }
        game_reset(&g,&t);
        /* Regression: full-power launch must leave the shooter lane and
         * reach a scoring bumper in the supplied Windows Space Cadet pack. */
        for(i=0;i<3000;i++) {
            unsigned keys=(i<PLUNGER_MAX_CHARGE?KEY_LAUNCH:0)|
                          (i%80<40?KEY_LEFT:KEY_RIGHT);
            game_step(&g,&t,keys);
            if(i==PLUNGER_MAX_CHARGE)
                printf("RELEASE: vy=%d charge=%d rng=%u\n",g.vy,g.charge,g.plunger_rng);
            if(g.waiting && i>PLUNGER_MAX_CHARGE+1) break;
        }
        printf("ORIGINAL: hits=%lu score=%lu steps=%d x=%d y=%d\n",
               g.hits,g.score,i,g.x/FP,g.y/FP); fflush(stdout);
        assert(g.hits>0 && g.score>=500);
        game_reset(&g,&t);
    }
    puts("STAGE: starting soak"); fflush(stdout);
    for(i=0;i<20000;i++) {
        unsigned keys=(i%192<48?KEY_LAUNCH:0)|(i%80<40?KEY_LEFT:KEY_RIGHT);
        if(g.over) game_reset(&g,&t);
        game_step(&g,&t,keys);
        if(!(g.x>=0 && g.x<240*FP && g.y>=0 && g.y<200*FP &&
             abs(g.vx)<1024 && abs(g.vy)<1024)) {
            printf("SOAK20 BAD step=%d xy=%d,%d velocity=%d,%d kick=%d delay=%d side=%d balls=%d\n",
                   i,g.x,g.y,g.vx,g.vy,g.kick_flight,g.kick_delay,
                   g.kick_side,g.balls); fflush(stdout);
        }
        assert(g.x>=0 && g.x<240*FP && g.y>=0 && g.y<200*FP);
        assert(abs(g.vx)<1024 && abs(g.vy)<1024);
    }
    puts("STAGE: 20000-step soak completed"); fflush(stdout);
    /* Multiple full-charge games and varied controls: invariants after
     * EVERY step, rather than only a final screenshot or screen bounds. */
    {
        int seed,step,side,k;
        unsigned random;
        for(seed=1;seed<=8;seed++) {
            game_reset(&g,&t); g.plunger_rng=(unsigned short)seed;
            random=(unsigned)seed;
            for(step=0;step<10000;step++) {
                unsigned keys;
                random=(unsigned short)(random*25173U+13849U);
                keys=(random>>8)&(KEY_LEFT|KEY_RIGHT);
                if(g.waiting && g.charge<PLUNGER_MAX_CHARGE) keys|=KEY_LAUNCH;
                if(step%503==0) keys|=KEY_NUDGE;
                if(g.over) game_reset(&g,&t);
                game_step(&g,&t,keys);
                if(!(g.x>=2*FP && g.x<=237*FP && g.y>=2*FP && g.y<200*FP)) {
                    printf("SOAK80 BOUNDS seed=%d step=%d xy=%d,%d kick=%d side=%d\n",
                           seed,step,g.x,g.y,g.kick_flight,g.kick_side);
                    fflush(stdout);
                }
                assert(g.x>=2*FP && g.x<=237*FP && g.y>=2*FP && g.y<200*FP);
                if(g.waiting || g.in_shooter || g.over) continue;
                for(side=0;side<2;side++) {
                    int index=side?t.outer_right_rail:t.outer_left_rail;
                    const Edge *e=&t.edges[index];
                    int low=e->y<e->ey?e->y:e->ey;
                    int high=e->y>e->ey?e->y:e->ey;
                    if(g.y/FP>=low && g.y/FP<=high) {
                        long limit=(long)e->x*FP+
                            ((long)g.y-(long)e->y*FP)*(e->ex-e->x)/(e->ey-e->y);
                        if(!(side ? g.x<=limit-2*FP : g.x>=limit+2*FP)) {
                            printf("SOAK80 RAIL seed=%d step=%d side=%d xy=%d,%d limit=%ld kick=%d\n",
                                   seed,step,side,g.x,g.y,limit,g.kick_flight);
                            fflush(stdout);
                        }
                        assert(side ? g.x<=limit-2*FP : g.x>=limit+2*FP);
                    }
                    if(overlaps_sling(&g,&t.slings[side])) {
                        printf("INVALID SLING seed=%d step=%d side=%d xy=%d,%d v=%d,%d shooter=%d\n",seed,step,side,g.x,g.y,g.vx,g.vy,g.in_shooter);
                        fflush(stdout);
                    }
                    assert(!overlaps_sling(&g,&t.slings[side]));
                }
                for(k=0;k<t.bumper_count;k++) {
                    const Bumper *b=&t.bumpers[k];
                    long dx=(long)g.x-b->x,dy=(long)g.y-b->y;
                    if(dx*dx+dy*dy<(long)b->radius*b->radius) {
                        printf("SOAK80 BUMPER seed=%d step=%d bumper=%d xy=%d,%d center=%d,%d r=%d kick=%d\n",
                               seed,step,k,g.x,g.y,b->x,b->y,b->radius,g.kick_flight);
                        fflush(stdout);
                    }
                    assert(dx*dx+dy*dy>=(long)b->radius*b->radius);
                }
            }
        }
        puts("STAGE: 80000 varied-control steps stay within rails and outside solids");
        fflush(stdout);
    }
    f=fopen("TEST.OK","w"); assert(f);
    fputs("PASS: plunger, one-way chute, shooter return, three-way nudge, tilt, timed kickbacks, drain, ball reset, swept walls, 20000-step soak, 80000-step containment\n",f);
    fclose(f); puts("All DOS core tests passed.");
    return 0;
}
