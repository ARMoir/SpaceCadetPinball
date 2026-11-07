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
    t.edges[0].x=100; t.edges[0].ex=100;
    t.edges[0].y=20; t.edges[0].ey=120;
    t.edges[0].ty=256; t.edges[0].length=100*FP;
    assert(table_index(&t)); game_reset(&g,&t);
}

int main(int argc,char **argv)
{
    int i;
    FILE *f;
    fixture();
    assert(integer_sqrt(0)==0 && integer_sqrt(1)==1);
    assert(integer_sqrt(65535UL)==255 && integer_sqrt(4294967295UL)==65535UL);
    assert(g.balls==3 && g.waiting);
    for(i=0;i<48;i++) game_step(&g,&t,KEY_LAUNCH);
    assert(g.waiting && g.charge==48);
    game_step(&g,&t,0); assert(!g.waiting && g.vy==-124);
    fixture(); g.waiting=0; g.x=97*FP; g.y=60*FP; g.vx=128;
    game_step(&g,&t,0); assert(g.vx<0 && g.x<100*FP);
    fixture(); t.edges[0].score=500; t.edges[0].boost=35;
    g.waiting=0; g.x=97*FP; g.y=60*FP; g.vx=128;
    game_step(&g,&t,0); assert(g.score==500 && g.hits==1);
    game_step(&g,&t,0); assert(g.score==500);
    fixture(); g.waiting=0; g.x=90*FP; g.y=163*FP; g.vy=30;
    game_step(&g,&t,KEY_LEFT); assert(g.vy<0);
    fixture(); g.waiting=0; g.x=140*FP; g.y=163*FP; g.vy=30;
    game_step(&g,&t,KEY_RIGHT); assert(g.vy<0);
    fixture(); g.waiting=0; g.x=50*FP; g.y=60*FP;
    game_step(&g,&t,KEY_NUDGE); assert(g.vy<0 && g.vx>0 && g.nudge_delay==72);
    game_step(&g,&t,0); game_step(&g,&t,KEY_NUDGE);
    assert(g.nudge_delay==70);
    /* A failed launch may roll back past the shared drain height.
     * It must return to the plunger without consuming a ball. */
    fixture(); g.waiting=0; g.x=(t.launch_x+2)*FP;
    g.y=(t.drain_y+1)*FP; g.vy=32;
    game_step(&g,&t,0);
    assert(g.balls==3 && g.waiting && !g.over && g.event==0);
    assert(g.x==t.launch_x*FP && g.y==t.launch_y*FP);
    assert(g.vx==0 && g.vy==0 && g.charge==0);
    game_step(&g,&t,KEY_LAUNCH);
    game_step(&g,&t,0);
    assert(!g.waiting && g.vy<0 && g.event==2);
    /* The adjacent main playfield still drains normally. */
    fixture(); g.waiting=0; g.x=(t.launch_x-12)*FP;
    g.y=(t.drain_y+1)*FP; g.vy=32;
    game_step(&g,&t,0);
    assert(g.balls==2 && g.waiting && g.event==3);
    fixture();
    for(i=0;i<3;i++) {
        g.waiting=0; g.y=192*FP; g.x=110*FP;
        game_step(&g,&t,0); assert(g.balls==2-i);
    }
    assert(g.over);
    game_reset(&g,&t); assert(g.score==0 && !g.over && g.balls==3);
    /* Exercise the actual imported pack under 16-bit arithmetic. */
    assert(table_load(&t,"TABLE.DAT")); game_reset(&g,&t);
    if(argc>1 && !strcmp(argv[1],"/original")) {
        /* Regression: full-power launch must leave the shooter lane and
         * reach a scoring bumper in the supplied Windows Space Cadet pack. */
        for(i=0;i<3000;i++) {
            unsigned keys=(i<48?KEY_LAUNCH:0)|(i%80<40?KEY_LEFT:KEY_RIGHT);
            game_step(&g,&t,keys);
            if(g.waiting && i>49) break;
        }
        assert(g.hits>0 && g.score>=500);
        game_reset(&g,&t);
    }
    for(i=0;i<20000;i++) {
        unsigned keys=(i%192<48?KEY_LAUNCH:0)|(i%80<40?KEY_LEFT:KEY_RIGHT);
        if(g.over) game_reset(&g,&t);
        game_step(&g,&t,keys);
        assert(g.x>=0 && g.x<240*FP && g.y>=0 && g.y<200*FP);
        assert(abs(g.vx)<1024 && abs(g.vy)<1024);
    }
    f=fopen("TEST.OK","w"); assert(f);
    fputs("PASS: launch, reflection, bumper score, both flippers, shooter return, drain, restart, 20000-step soak\n",f);
    fclose(f); puts("All DOS core tests passed.");
    return 0;
}
