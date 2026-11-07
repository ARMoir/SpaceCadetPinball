/* SOURCE MAP: Kickback catch
 * ORIGINAL: TKickback.cpp; control.cpp
 * STATUS: ADAPTED
 * MATCHES: source strike center and delayed firing
 * DIVERGES: automatic one-use arming; missing gate targets
 * EXCEPTIONS: F-001
 * See SOURCE_MAP.md and FIDELITY.md.
 * One implementation fragment included by its owning C translation unit.
 */
/* The source TKickback winds up before striking, then its gate closes.
 * In DOS the original target triplets are not imported yet: allow one
 * out-lane save per side per ball, not endless free saves. The centers are
 * imported from the source strike segments. Trigger widths, timing and
 * impulses remain approximations until original target-state gates exist. */
static int catch_kickback(Game *g,const Table *t)
{
    int i,px=g->x/FP,py=g->y/FP;
    if(g->in_shooter || g->tilted || g->vy<=0)
        return 0;
    for(i=0;i<2;i++) {
        int cx=t->kickback[i*2],cy=t->kickback[i*2+1];
        if(g->kick_armed[i] && abs(px-cx)<=7 && abs(py-cy)<=5) {
            g->kick_armed[i]=0;
            g->kickbacks++;
            g->kick_side=(unsigned char)(i+1);
            g->kick_delay=9; /* 0.12s / 72.8Hz, short 8086-friendly wind-up */
            g->x=(short)(cx*FP); g->y=(short)(cy*FP);
            g->vx=g->vy=0;
            return 1;
        }
    }
    return 0;
}

