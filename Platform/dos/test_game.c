#include "game.h"
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

int main(int argc,char **argv)
{
    int i,weak_vy,full_vy;
    FILE *f;
    fixture();
    /* INT 33h button bits feed the existing flipper controls; gameplay
     * remains exactly the same for keyboard or mouse source. */
    assert(MOUSE_FLIPPER_KEYS(0)==0);
    assert(MOUSE_FLIPPER_KEYS(1)==KEY_LEFT);
    assert(MOUSE_FLIPPER_KEYS(2)==KEY_RIGHT);
    assert(MOUSE_FLIPPER_KEYS(3)==(KEY_LEFT|KEY_RIGHT));
    assert(MOUSE_FLIPPER_KEYS(4)==0); /* middle button ignored */
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
           g.event==4 && g.shake_frames>0);
    g.x=t.kickback[0]*FP;
    g.y=t.kickback[1]*FP; g.vy=32;
    game_step(&g,&t,0);
    assert(!g.kick_delay && g.kickbacks==1);
    fixture(); g.waiting=0; g.in_shooter=0;
    g.x=t.kickback[2]*FP; g.y=t.kickback[3]*FP; g.vy=32;
    game_step(&g,&t,0);
    assert(g.kick_delay==9 && g.kick_side==2 && g.kickbacks==1);
    for(i=0;i<9;i++) game_step(&g,&t,0);
    assert(g.kick_delay==0 && g.vx<0 && g.vy<0);
    puts("STAGE: both kickbacks passed"); fflush(stdout);
    /* A kickback must not disable unrelated walls during its first steps. */
    fixture();
    t.edges[0].x=40; t.edges[0].ex=70;
    t.edges[0].y=t.edges[0].ey=160;
    t.edges[0].tx=256; t.edges[0].ty=0; t.edges[0].length=30*FP;
    assert(table_index(&t));
    g.waiting=0; g.in_shooter=0;
    g.x=t.kickback[0]*FP; g.y=t.kickback[1]*FP; g.vy=32;
    game_step(&g,&t,0);
    assert(g.kick_delay==9);
    for(i=0;i<13;i++) game_step(&g,&t,0);
    assert(g.vy>0 && g.y>160*FP);
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
                assert(g.x==t.launch_x*FP && g.y==t.launch_y*FP);
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
            g.steps=97;
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
        assert(g.x>=0 && g.x<240*FP && g.y>=0 && g.y<200*FP);
        assert(abs(g.vx)<1024 && abs(g.vy)<1024);
    }
    f=fopen("TEST.OK","w"); assert(f);
    fputs("PASS: plunger, one-way chute, shooter return, three-way nudge, tilt, timed kickbacks, drain, ball reset, 20000-step soak\n",f);
    fclose(f); puts("All DOS core tests passed.");
    return 0;
}
