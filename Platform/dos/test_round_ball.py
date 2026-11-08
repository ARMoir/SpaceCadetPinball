#!/usr/bin/env python3
"""F-007: round ball silhouette, source comparison, dual-video compatibility."""
import re
import unittest
from pathlib import Path

BASE=Path(__file__).resolve().parent
BALL=(BASE/"round_ball.inc").read_text()
MAIN=(BASE/"main.c").read_text()
VIDEO=(BASE/"video.inc").read_text()
ASSETS=(BASE/"assets.inc").read_text()


class RoundBallTests(unittest.TestCase):
    def test_symmetric_round_disc(self):
        self.assertIn("#define ROUND_BALL_W 6",BALL)
        self.assertIn("#define ROUND_BALL_H 5",BALL)
        m=re.search(r"round_pixels\[ROUND_BALL_W\*ROUND_BALL_H\]\s*=\s*\{([^}]+)\}",BALL,re.S)
        self.assertIsNotNone(m)
        nums=[int(x) for x in re.findall(r"\d+",m.group(1))]
        self.assertEqual(len(nums),30)
        rows=[nums[i:i+6] for i in range(0,30,6)]
        self.assertEqual([sum(x!=255 for x in row) for row in rows],[2,4,6,4,2])
        self.assertEqual(rows,list(reversed(rows)))
        self.assertTrue(all(row==list(reversed(row)) for row in rows))
        self.assertEqual(set(nums),{8,15,255})
        self.assertEqual(rows[2],[8,15,15,15,15,8])
        self.assertEqual(rows[0],[255,255,8,8,255,255])

    def test_default_and_comparison_switch(self):
        self.assertIn('else if(!strcmp(argv[i],"/sourceball")) source_ball=1;',MAIN)
        self.assertIn("if(!source_ball) install_round_ball(&table.sprites[4]);",MAIN)
        self.assertGreater(MAIN.index("install_round_ball(&table.sprites[4])"),
                           MAIN.index("if(!load_background(&enhanced))"))
        self.assertIn("memcpy(ball->pixels,round_pixels,sizeof(round_pixels))",BALL)
        self.assertIn("ball->w=ROUND_BALL_W;",BALL)
        self.assertIn("ball->h=ROUND_BALL_H;",BALL)
        self.assertIn("fread(s->pixels,1,s->w*s->h,f)",ASSETS)
        self.assertNotIn("TABLE.DAT",MAIN[MAIN.index("if(!source_ball)"):MAIN.index("load_menu_background()")])
        
    def test_video_dirty_rect_and_no_shadow(self):
        self.assertIn("sprite(&table.sprites[4],old_x+1,old_y+1);",MAIN)
        self.assertIn("table.sprites[4].w+2",MAIN)
        self.assertIn("table.sprites[4].h+2",MAIN)
        self.assertIn("table.sprites[4].w/2-1",MAIN)
        self.assertIn("table.sprites[4].h/2-1",MAIN)
        self.assertIn("if(vga_mode)",VIDEO)
        self.assertIn("if(p[xx]!=255) video[a]=p[xx];",VIDEO)
        self.assertNotIn("pixel(old_x+1",MAIN)
        self.assertNotIn("pixel(old_x+2",MAIN)

if __name__=="__main__":
    unittest.main()
