/* Native 8086 real-mode DOS: Tandy mode 09h and standard VGA mode 13h. */
#include "game.h"
#include "scores.h"
#include "menu_art.h"
#include "score_font.h"
#include <dos.h>
#include <conio.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Table table;
static Game game;

/* F-002/F-006 flight recorder: preserve ~14 seconds of gameplay state in
 * 256 frames. This captures key/control state and passive vs powered flipper
 * contacts, which a cumulative INPUT.TXT mask cannot explain. No file I/O
 * occurs inside gameplay; /record writes the ring buffer only on exit. */
typedef struct {
    unsigned short frame,late_frames;
    unsigned long steps,score;
    short x,y,vx,vy;
    unsigned short border,right_rail,left_rail,flips[2],powered[2];
    unsigned char input,balls,waiting,in_shooter,event,charge;
    unsigned char state,flipper_sweep[2];
} FlightTrace;
static FlightTrace flight[256];
static unsigned flight_next,flight_used;
static unsigned char __far *background;
static unsigned char __far *video;
static volatile unsigned char keys[128];
static volatile unsigned char pressed_keys[128];
static void (__interrupt __far *old_keyboard)(void);
static int sound_enabled=1, sound_ticks;
static int lights_enabled=1;
static int shake_enabled=1,video_shift=0;
static int mouse_available=0; /* Only poll INT 33h when a driver exists. */
static int vga_colors=0,rgbi_mode=0;
static int vga_mode=0; /* Selected once before display setup. */

/* Platform fragments retain one DOS translation unit and their original
 * call order. Only source organization changed; not video/input behavior. */
#include "video_shake.inc"
#include "input.inc"
#include "video.inc"
#include "hud.inc"
#include "audio.inc"
#include "lamps.inc"
#include "assets.inc"
#include "round_ball.inc"
int main(int argc,char **argv)
{
    unsigned last,now,frame=0,input,previous_input=0;
    unsigned long old_score=~0UL,slow_frames=0;
    int i,smoke=0,record=0,enhanced=1,old_x=-10,old_y=-10,old_balls=-1,old_wait=-1,old_over=-1;
    int old_new=0,event,next_x,next_y,ball_changed,ball_redraw;
    int event_bumper=0,sound_event,lights_damage;
    unsigned short prev_nudges=0,prev_slings[2]={0,0};
    int plunger_x,plunger_y,old_plunger_x=-1,old_plunger_y=-1;
    int plunger_changed,plunger_redraw,plunger_charge;
    int plunger_frame,old_plunger_frame=-1,old_plunger_w=0,old_plunger_h=0;
    const Sprite *plunger_sprite;
    int view=0,choice=0,paused=0,exit_requested=0,ui_dirty=0,prior_view=0;
    int hud_status_dirty=0; /* F-007: event text only, not full HUD */
    int old_flags=-1;
    unsigned char toggle_sound,toggle_menu,key_up,key_down;
    unsigned char key_enter,key_esc,key_f1,key_f2,key_f3,key_f5,key_f8,key_new;
    unsigned char key_backspace;
    unsigned score_scan;
    unsigned observed=0,launches=0,full_launches=0,mouse_input=0;
    int no_mouse=0,video_choice=-1,source_ball=0; /* -1 detect VGA */
    union REGS r;
    unsigned char old_mode;
    FILE *report;
    for(i=1;i<argc;i++) {
        if(!strcmp(argv[i],"/smoke")) smoke=1;
        else if(!strcmp(argv[i],"/record")) record=1;
        else if(!strcmp(argv[i],"/nosound")) sound_enabled=0;
        else if(!strcmp(argv[i],"/nolights")) lights_enabled=0;
        else if(!strcmp(argv[i],"/noshake")) shake_enabled=0;
        else if(!strcmp(argv[i],"/nomouse")) no_mouse=1;
        else if(!strcmp(argv[i],"/tandy")) video_choice=0;
        else if(!strcmp(argv[i],"/vga")) video_choice=1;
        else if(!strcmp(argv[i],"/rgbi")) rgbi_mode=1;
        else if(!strcmp(argv[i],"/sourceball")) source_ball=1;
        else if(!strcmp(argv[i],"/classic")) enhanced=0;
        else if(!strcmp(argv[i],"/3d")) enhanced=1;
        else { puts("Usage: CADET [/smoke] [/record] [/nosound] [/nolights] [/noshake] [/nomouse] [/tandy|/vga] [/classic|/3d] [/rgbi] [/sourceball]"); return 1; }
    }
    if(sizeof(short)!=2 || sizeof(long)!=4 || sizeof(Edge)!=18) return 1;
    if(video_choice==1 && !detect_vga()) {
        puts("VGA BIOS not detected; /vga requires standard VGA hardware.");
        return 1;
    }
    vga_mode=(video_choice==1 ||
              (video_choice<0 && detect_vga()));
    if(!table_load(&table,"TABLE.DAT")) {
        puts("Missing or invalid TABLE.DAT. Run convert.py or make_demo.py first."); return 1;
    }
    background=_fmalloc(vga_mode?64768UL:32768UL);
    if(!background) {
        puts("Not enough conventional memory for selected video background.");
        return 1;
    }
    if(!load_background(&enhanced)) {
        _ffree(background);
        puts("Missing or invalid table artwork (32768 bytes).");
        return 1;
    }
    /* Install the presentation-only round sprite after SCP2/VGA1
     * validation. Gameplay/collision radius, coordinates, imported
     * assets and plunger seating stay unchanged. */
    if(!source_ball) install_round_ball(&table.sprites[4]);
    /* MENU.DAT is generated offline from the owner's title image.
     * Optional: legacy UI remains available if the pack is missing. */
    load_menu_background();
    game_reset(&game,&table);
    lamps_reset(&game);
    /* Read once, before installing the keyboard IRQ hook. /smoke avoids
     * all high-score I/O so deterministic emulator tests never write it. */
    scores_reset();
    if(!smoke) scores_load("SCORES.DAT");
    if(!smoke) game.plunger_rng^=ticks(); /* Reproducible smoke, varied interactive play. */
    memset(&r,0,sizeof(r)); r.h.ah=15; int86(0x10,&r,&r); old_mode=r.h.al;
    mode((unsigned char)(vga_mode?0x13:9));
    if(vga_mode) set_vga_palette();
    /* Enable mouse only if a standard DOS mouse driver is present.
     * Smoke tests stay deterministic even in emulators with built-in mice. */
    if(!smoke && !no_mouse) mouse_available=mouse_detect();
    video=(unsigned char __far *)MK_FP(vga_mode?0xa000:0xb800,0);
    _fmemcpy(video,background,vga_mode?64000U:32768U);
    draw_title();
    draw_panel(view,choice,paused);
    silence();
    old_keyboard=_dos_getvect(9);
    _disable(); _dos_setvect(9,keyboard); _enable();
    last=ticks();
    while(!exit_requested && (!smoke || frame<360)) {
        now=ticks();
        if(now==last) continue;
        if((unsigned)(now-last)>1) slow_frames++;
        last=now; frame++;
        /* Latch quick taps that start and end between 18.2 Hz frames. */
        _disable();
        input=((keys[44]||pressed_keys[44])?KEY_LEFT:0)|
              ((keys[53]||pressed_keys[53])?KEY_RIGHT:0)|
              ((keys[57]||pressed_keys[57])?KEY_LAUNCH:0)|
              ((keys[45]||pressed_keys[45])?KEY_NUDGE:0)|
              ((keys[30]||pressed_keys[30])?KEY_NUDGE_LEFT:0)|
              ((keys[32]||pressed_keys[32])?KEY_NUDGE_RIGHT:0);
        pressed_keys[44]=pressed_keys[53]=pressed_keys[57]=pressed_keys[45]=0;
        pressed_keys[30]=pressed_keys[32]=0;
        toggle_sound=pressed_keys[31]; /* S */
        toggle_menu=pressed_keys[50]; /* M */
        key_up=pressed_keys[72]; key_down=pressed_keys[80];
        key_enter=pressed_keys[28]; key_esc=pressed_keys[1];
        key_backspace=pressed_keys[14];
        key_f1=pressed_keys[59]; key_f2=pressed_keys[60];
        key_f3=pressed_keys[61]; key_f5=pressed_keys[63];
        key_f8=pressed_keys[66]; key_new=pressed_keys[49];
        pressed_keys[31]=pressed_keys[50]=pressed_keys[72]=0;
        pressed_keys[80]=pressed_keys[28]=pressed_keys[1]=0;
        pressed_keys[59]=pressed_keys[60]=pressed_keys[61]=0;
        pressed_keys[63]=pressed_keys[66]=pressed_keys[14]=0;
        pressed_keys[49]=0;
        _enable();
        /* OR mouse with existing Z and / controls; either device can
         * hold or trigger either flipper without altering physics. */
        /* Read INT 33h once. Passing mouse_buttons() directly into the
         * macro would repeat the driver call and consume press counters. */
        mouse_input=mouse_available?mouse_buttons():0;
        input|=MOUSE_GAME_KEYS(mouse_input);
        ui_dirty=0; hud_status_dirty=0;
        if(smoke) {
            /* Hold until fully charged, then release; fixed short pulses
             * never reached the 96-substep pullback and hid failed shots.
             * Match the deterministic full-shot regression's flipper phase. */
            input=((frame-1)%20<10?KEY_LEFT:KEY_RIGHT);
            if(game.waiting && game.charge<PLUNGER_MAX_CHARGE)
                input|=KEY_LAUNCH;
        }
        observed|=input;
        if(!score_capture && (toggle_sound || key_f5)) {
            /* Mute before disabling the PSG, so a held note cannot stick. */
            if(sound_enabled) { silence(); sound_ticks=0; sound_enabled=0; }
            else { sound_enabled=1; sound_ticks=0; }
            ui_dirty=1;
        }
        if(view==4) {
            /* Initials may contain N. Do not treat the same N key as NEW
             * GAME if Enter also confirms the score on this frame. */
            key_new=0; key_f2=0;
            /* The ball is already over and physics is frozen. Process every
             * queued letter in press order, including rapid XT key taps. */
            while((score_scan=score_next_scan())!=0) {
                char ch=score_letter(score_scan);
                if(ch && score_name_length<3) {
                    score_name[score_name_length++]=ch;
                    ui_dirty=1;
                }
            }
            if(key_backspace && score_name_length) {
                score_name[--score_name_length]='A';
                ui_dirty=1;
            }
            if(key_enter) {
                scores_insert(game.score,score_name);
                score_save_failed=!scores_save("SCORES.DAT");
                score_capture=0;
                view=3; ui_dirty=1;
            } else if(key_esc) {
                score_capture=0; view=0; ui_dirty=1;
            }
        } else {
            if(toggle_menu) { view=view?0:1; ui_dirty=1; }
            if(key_f1 || key_f8) { view=2; ui_dirty=1; }
            if(key_esc) {
                if(view==2 || view==3) view=1;
                else if(view) view=0;
                else exit_requested=1;
                ui_dirty=1;
            }
            if(view==1) {
                if(key_up) { choice=(choice+5)%6; ui_dirty=1; }
                if(key_down) { choice=(choice+1)%6; ui_dirty=1; }
                if(key_enter) {
                    ui_dirty=1;
                    if(choice==0) key_new=1;
                    else if(choice==1) { paused=!paused; view=0; }
                    else if(choice==2) {
                        if(sound_enabled) { silence(); sound_ticks=0; sound_enabled=0; }
                        else { sound_enabled=1; sound_ticks=0; }
                    } else if(choice==3) view=2;
                    else if(choice==4) view=3;
                    else exit_requested=1;
                }
            } else if((view==2 || view==3) && key_enter) {
                view=1; ui_dirty=1;
            }
            if(key_f3) { paused=!paused; ui_dirty=1; }
        }
        if(view!=4 && (key_f2 || key_new)) {
            game_reset(&game,&table);
            lamps_reset(&game);
            prev_nudges=prev_slings[0]=prev_slings[1]=0;
            game.plunger_rng^=ticks();
            paused=0; view=0; ui_dirty=1;
        }
        old_new=keys[49];
        event=0; event_bumper=0;
        /* Keeping physics frozen is important for a faithful Game menu
         * and for testing on a slow V20. No catch-up on resume. */
        if(!view && !paused && !exit_requested) {
            for(i=0;i<4;i++) {
                game_step(&game,&table,input);
                if(game.event) {
                    event=game.event;
                    if(event==1) event_bumper=game.event_bumper;
                    if(event==2) {
                        launches++;
                        if(game.charge==PLUNGER_MAX_CHARGE) full_launches++;
                    }
                }
            }
        } else input=0;
        /* Match the unpowered resting collision surface during tilt. */
        if(game.tilted) input&=~(KEY_LEFT|KEY_RIGHT);
        /* Physical actions not represented by the five original compact
         * DOS event codes still have short audible feedback. Never allow
         * a flipper tap to obscure a higher-priority scoring/tilt cue. */
        sound_event=event;
        if(!sound_event && !view && !paused && !game.over) {
            if(game.sling_rescues[0]!=prev_slings[0] ||
               game.sling_rescues[1]!=prev_slings[1])
                sound_event=8;
            else if(game.nudges!=prev_nudges) sound_event=7;
            else if((input&~previous_input)&(KEY_LEFT|KEY_RIGHT))
                sound_event=6;
        }
        prev_nudges=game.nudges;
        prev_slings[0]=game.sling_rescues[0];
        prev_slings[1]=game.sling_rescues[1];
        lamps_advance(&game,event==1?event_bumper:0,!view&&!paused);
        play_event(sound_event,(unsigned)event_bumper);
        /* TTextBox-inspired single transient message. Only UI changes,
         * with no source physics or rendering asset modifications. */
        if(event) {
            /* Keep the current message if another matching event fires.
             * A repeated bumper score only needs its score digits. */
            if(!hud_event_frames || hud_event!=event)
                hud_status_dirty=1;
            hud_event=event; hud_event_frames=26;
        } else if(hud_event_frames && !view && !paused) {
            if(!--hud_event_frames) hud_status_dirty=1;
        }
        /* Detect a completed qualifying game exactly once. Keyboard/menu
         * entry and subsequent score file writes occur with physics stopped.
         * Never start score entry on the deterministic /smoke path. */
        if(!smoke && old_over==0 && game.over &&
           scores_rank(game.score)>=0) {
            score_name[0]=score_name[1]=score_name[2]='A';
            score_name[3]=0; score_name_length=0;
            score_head=score_tail=0;
            score_capture=1;
            view=4; ui_dirty=1;
        }
        if(record) {
            FlightTrace *sample=&flight[flight_next];
            sample->frame=frame;
            sample->late_frames=(unsigned short)slow_frames;
            sample->steps=game.steps; sample->score=game.score;
            sample->state=(unsigned char)((paused?1:0)|(view?2:0)|
                                        (game.tilted?4:0)|(game.over?8:0)|
                                        ((mouse_input&KEY_LEFT)?16:0)|
                                        ((mouse_input&KEY_RIGHT)?32:0));
            sample->flipper_sweep[0]=game.sweep[0];
            sample->flipper_sweep[1]=game.sweep[1];
            sample->x=game.x; sample->y=game.y;
            sample->vx=game.vx; sample->vy=game.vy;
            sample->input=(unsigned char)input;
            sample->flips[0]=game.flipper_contacts[0];
            sample->flips[1]=game.flipper_contacts[1];
            sample->powered[0]=game.flipper_power_contacts[0];
            sample->powered[1]=game.flipper_power_contacts[1];
            sample->balls=(unsigned char)game.balls;
            sample->waiting=(unsigned char)game.waiting;
            sample->in_shooter=game.in_shooter;
            sample->charge=(unsigned char)game.charge;
            sample->event=(unsigned char)event;
            sample->border=game.boundary_hits;
            sample->right_rail=game.right_rail_saves;
            sample->left_rail=game.left_rail_saves;
            flight_next=(flight_next+1)&255;
            if(flight_used<256) flight_used++;
        }
        /* Visible shake comes from the CRTC, never a memory-wide screen
         * redraw. The offset alternates briefly and resets on pause/menu. */
        camera_shift(!view && !paused && !exit_requested &&
                     ((game.shake_frames/4)&1)?1:0);
        /* A menu is a paused scene, not a background screen refresh.
         * Show the owner's 240x200 splash only when entering the menu.
         * Never redraw sprites over it or do any disk I/O per frame. */
        if(view) {
            if(!prior_view && menu_background) draw_menu_background();
            if(ui_dirty || prior_view!=view) draw_panel(view,choice,paused);
            prior_view=view;
            old_score=game.score; old_balls=game.balls;
            old_wait=game.waiting; old_over=game.over;
            previous_input=0;
            continue;
        }
        if(prior_view) {
            /* Restore the entire original playfield exactly once and
             * redraw every moving sprite after the menu disappears. */
            restore(0,0,320,200);
            draw_title();
            lamps_invalidate();
            old_x=-10; old_y=-10; old_plunger_y=-1;
            old_plunger_frame=-1;
            previous_input=input^3U; /* Force both flipper states to redraw. */
            old_score=~0UL;
            old_balls=old_wait=old_over=old_flags=-1;
            prior_view=0; ui_dirty=1;
        }
        plunger_charge=game.waiting?game.charge:0;
        plunger_frame=plunger_charge*7/PLUNGER_MAX_CHARGE;
        plunger_sprite=&table.plunger[plunger_frame];
        plunger_x=plunger_sprite->x; plunger_y=plunger_sprite->y;
        plunger_changed=(plunger_frame!=old_plunger_frame);
        if(plunger_changed && old_plunger_y>=0)
            restore(old_plunger_x,old_plunger_y,old_plunger_w,old_plunger_h);
        next_x=game.x/FP-table.sprites[4].w/2-1;
        next_y=game.y/FP-table.sprites[4].h/2-1;
        /* Positions are Q6 but only whole screen pixels are displayed.
         * Do not erase/repaint a ball that has not moved onscreen.
         * This also prevents an 18 Hz erase/repaint flash at the plunger. */
        ball_changed=(next_x!=old_x || next_y!=old_y || old_over!=game.over);
        ball_redraw=ball_changed;
        if(plunger_changed &&
           (rect_overlaps(next_x,next_y,
                          table.sprites[4].w+2,table.sprites[4].h+2,
                          plunger_x,plunger_y,plunger_sprite->w,plunger_sprite->h) ||
            (old_plunger_y>=0 &&
             rect_overlaps(next_x,next_y,
                           table.sprites[4].w+2,table.sprites[4].h+2,
                           old_plunger_x,old_plunger_y,old_plunger_w,old_plunger_h))))
            ball_redraw=1;
        if(ball_changed)
            restore(old_x,old_y,table.sprites[4].w+2,table.sprites[4].h+2);
        for(i=0;i<2;i++) {
            const Sprite *s;
            int changed=(input^previous_input)&(1<<i);
            if(changed) {
                s=&table.sprites[i*2]; restore(s->x-1,s->y-1,s->w+2,s->h+2);
                s++; restore(s->x-1,s->y-1,s->w+2,s->h+2);
            }
            s=&table.sprites[i*2+((input&(1<<i))!=0)];
            if(frame==1 || changed ||
               (ball_changed && ball_overlaps(s,old_x,old_y,
                            table.sprites[4].w+2,table.sprites[4].h+2)))
                sprite(s,s->x,s->y);
            /* Changing a flipper can erase a stationary ball that overlaps
             * it. Repaint the ball once after the flipper when necessary. */
            if(changed &&
               (ball_overlaps(&table.sprites[i*2],next_x,next_y,
                              table.sprites[4].w+2,table.sprites[4].h+2) ||
                ball_overlaps(&table.sprites[i*2+1],next_x,next_y,
                              table.sprites[4].w+2,table.sprites[4].h+2)))
                ball_redraw=1;
        }
        /* The old ball restore may erase the stationary plunger; only
         * repaint if their rectangles actually overlap. Paint ball last. */
        plunger_redraw=plunger_changed ||
           (ball_changed &&
            rect_overlaps(old_x,old_y,
                          table.sprites[4].w+2,table.sprites[4].h+2,
                          plunger_x,plunger_y,plunger_sprite->w,plunger_sprite->h));
        /* Lamp transitions restore only 5x5 source-art patches. Repaint
         * foreground sprites if one overlapped; the ball stays topmost. */
        if(lights_enabled) {
            lights_damage=lamps_render(&table,&game,
                old_x,old_y,next_x,next_y,
                table.sprites[4].w+2,table.sprites[4].h+2,
                ball_changed,plunger_x,plunger_y,
                plunger_sprite->w,plunger_sprite->h);
            if(lights_damage&1) ball_redraw=1;
            if(lights_damage&2) plunger_redraw=1;
        }
        /* The TGate collision/closed metal leaf is independent from
         * the decorative lamp toggle. Paint before the moving ball. */
        if(kicker_gates_render(&table,&game,
                              old_x,old_y,next_x,next_y,
                              table.sprites[4].w+2,table.sprites[4].h+2,
                              ball_changed))
            ball_redraw=1;
        if(plunger_redraw) sprite(plunger_sprite,plunger_x,plunger_y);
        old_plunger_x=plunger_x; old_plunger_y=plunger_y;
        old_plunger_frame=plunger_frame;
        old_plunger_w=plunger_sprite->w; old_plunger_h=plunger_sprite->h;
        old_x=next_x; old_y=next_y;
        if(!game.over && ball_redraw) {
            /* F-007: source asymmetric 4x4 (optional /sourceball),
             * or default symmetric unshaded 6x5 circular sprite.
             * No extra shadow/glint; ball remains the topmost sprite. */
            sprite(&table.sprites[4],old_x+1,old_y+1);
        }
        {
            int flags=(game.tilted?1:0)|(game.tilt_warning?2:0)|
                      (game.kick_delay?4:0)|
                      (game.kick_armed[0]?8:0)|
                      (game.kick_armed[1]?16:0);
            /* F-007: score events never recompose the full sidebar.
             * Small non-overlapping patches preserve static frames and
             * source title art, with minimal packed VRAM traffic. */
            if(ui_dirty) {
                draw_panel(view,choice,paused);
            } else {
                if(old_score!=game.score) draw_score_only();
                if(old_balls!=game.balls) draw_ball_only();
                if(old_over!=game.over) draw_command_only(paused);
                if(hud_status_dirty || old_wait!=game.waiting ||
                   old_over!=game.over || old_flags!=flags)
                    draw_status_only();
            }
            old_flags=flags;
        }
        old_score=game.score; old_balls=game.balls;
        old_wait=game.waiting; old_over=game.over;
        previous_input=input;
    }
    score_capture=0;
    _disable(); _dos_setvect(9,old_keyboard); _enable();
    camera_shift(0);
    silence();
    if(smoke || record) dump_screen();
    mode(old_mode); _ffree(background);
    if(menu_background) _ffree(menu_background);
    if(smoke || record) {
        report=fopen(smoke?"SMOKE.TXT":"INPUT.TXT","w");
        if(!report) return 1;
        fprintf(report,"PASS frames=%u steps=%lu score=%lu hits=%lu balls=%d slow_frames=%lu input=%u launches=%u nudges=%u kickbacks=%u tilts=%u full_launches=%u boundary_hits=%u right_rail_saves=%u left_rail_saves=%u left_hits=%u right_hits=%u left_power=%u right_power=%u mouse_driver=%u video_vga=%u sling_left_rescues=%u sling_right_rescues=%u sling_loop_breaks=%u ramp_crest_assists=%u sling_backflow_redirects=%u unstick_nudges=%u unstick_moves=%u unstick_relaunches=%u\n",
                frame,game.steps,game.score,game.hits,game.balls,slow_frames,
                observed,launches,game.nudges,game.kickbacks,game.tilts,
                full_launches,game.boundary_hits,game.right_rail_saves,
                game.left_rail_saves,game.flipper_contacts[0],game.flipper_contacts[1],
                game.flipper_power_contacts[0],game.flipper_power_contacts[1],
                mouse_available,vga_mode,
                game.sling_rescues[0],game.sling_rescues[1],
                game.sling_loop_breaks,game.ramp_crest_assists,
                 game.sling_backflow_redirects,
                 game.unstick_nudges,game.unstick_moves,game.unstick_relaunches);
        fprintf(report,"# art=%s static_baked=1 moving_ball_shadow=0\n",
                enhanced?"3d":"classic");
        fprintf(report,"# spent_gate_blocks=%u\n",game.spent_gate_blocks);
        fprintf(report,"# oneway_blocks=%u oneway_passes=%u ramp_entries=%u\n",
                game.oneway_blocks,game.oneway_passes,game.ramp_entries);
        fprintf(report,"# vga_colors=%d wall_crossings=%u\n",vga_colors,game.wall_crossings);
        fclose(report);
    }
    if(record) {
        report=fopen("FLIGHT.TXT","w");
        if(report) {
            unsigned j;
            fprintf(report,"# CADET_FLIGHT schema=4 count=%u rate_hz=18.2 scale=64 video=%s\n",
                    flight_used,vga_mode?"vga13":"tandy09");
            fprintf(report,"# Q6 positions/velocities divide by 64; INPUT bits 1=Z 2=/ 4=SPACE 8=X 16=A 32=D\n");
            fprintf(report,"# STATE bits 1=PAUSE 2=MENU 4=TILT 8=GAME_OVER 16=MOUSE_LEFT 32=MOUSE_RIGHT; EVENT 0=none 1=bumper 2=launch 3=drain 4=kickback 5=tilt\n");
            fprintf(report,"# Contact counters (LH RH LP RP), boundary and late-frame counts are cumulative; SWEEP is remaining active substeps.\n");
            fprintf(report,"FRAME XQ6 YQ6 VXQ6 VYQ6 BALLS READY TUBE EVENT CHARGE BORDER RIGHT_RAIL LEFT_RAIL INPUT LEFT_HIT RIGHT_HIT LEFT_POWER RIGHT_POWER STEPS SCORE LATE STATE LEFT_SWEEP RIGHT_SWEEP\n");
            for(j=0;j<flight_used;j++) {
                unsigned ix=(flight_next+256-flight_used+j)&255;
                const FlightTrace *sample=&flight[ix];
                fprintf(report,"%u %d %d %d %d %u %u %u %u %u %u %u %u %u %u %u %u %u %lu %lu %u %u %u %u\n",
                        sample->frame,sample->x,sample->y,
                        sample->vx,sample->vy,
                        sample->balls,sample->waiting,sample->in_shooter,
                        sample->event,sample->charge,sample->border,
                        sample->right_rail,sample->left_rail,
                        sample->input,sample->flips[0],sample->flips[1],
                        sample->powered[0],sample->powered[1],
                        sample->steps,sample->score,sample->late_frames,
                        sample->state,sample->flipper_sweep[0],
                        sample->flipper_sweep[1]);
            }
            fclose(report);
        }
    }
    puts("Space Cadet DOS prototype. Original assets; simplified physics/rules.");
    return 0;
}
