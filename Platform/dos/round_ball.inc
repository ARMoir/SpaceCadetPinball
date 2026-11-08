/* F-007: ROUND silhouette is preferred to fake 3D at 320x200.
 * ORIGINAL: Windows PINBALL.DAT 4x4 ball in TABLE.DAT/VGA.DAT.
 * STATUS: ADAPTED (owner preference; see FIDELITY.md).
 *
 * A 320x200 pixel on a 4:3 CRT is about 20% taller than it is wide.
 * A symmetric 6x5 indexed mask therefore appears nearly circular,
 * while the original 4x4 metallic sprite has an asymmetric outline.
 *
 * Uniform dark-gray (8) outline, uniform white (15) fill; no shiny
 * corners, drop shadow, directional gradient or per-frame pixel work.
 * 255 = transparent. Both Tandy09 and VGA13 use shared RGBI indices.
 * Original TABLE.DAT/VGA.DAT sprite is untouched; /sourceball restores
 * it. The existing optimized sprite/dirty-rectangle renderer is reused.
 */
#define ROUND_BALL_W 6
#define ROUND_BALL_H 5
static void install_round_ball(Sprite *ball)
{
    static const unsigned char round_pixels[ROUND_BALL_W*ROUND_BALL_H]={
        255,255,  8,  8,255,255,
        255,  8, 15, 15,  8,255,
          8, 15, 15, 15, 15,  8,
        255,  8, 15, 15,  8,255,
        255,255,  8,  8,255,255
    };
    memcpy(ball->pixels,round_pixels,sizeof(round_pixels));
    ball->w=ROUND_BALL_W;
    ball->h=ROUND_BALL_H;
}
