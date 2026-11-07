#include "game.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int minimum(int a,int b) { return a<b?a:b; }
static int maximum(int a,int b) { return a>b?a:b; }

unsigned long integer_sqrt(unsigned long n)
{
    unsigned long root=0, bit=1UL<<30;
    while (bit>n) bit>>=2;
    while (bit) {
        if (n>=root+bit) { n-=root+bit; root=(root>>1)+bit; }
        else root>>=1;
        bit>>=2;
    }
    return root;
}

/* This one translation unit includes every hot-path implementation fragment,
 * preserving exact statement order and 8086 static-helper visibility. */
#include "state.inc"
#include "ball_collision.inc"
#include "bumper.inc"
#include "slingshot.inc"
#include "rail_collision.inc"
#include "kickback.inc"
#include "nudge.inc"
#include "unstuck.inc"
#include "control.inc"
