"""Regression checks for human-readable logs, including real-Tandy legacy format."""
import tempfile
import unittest
from pathlib import Path
from analyze_logs import parse_flight, parse_summary, summarize, summary_lines


OLD = """last 2 rendered frames (~14 seconds maximum); positions and velocities Q6
FRAME XQ6 YQ6 VXQ6 VYQ6 BALLS READY TUBE EVENT CHARGE BORDER
262 8086 11424 -1 45 3 0 0 0 96 0
263 11584 9024 0 0 2 1 1 3 0 0
"""

NEW = """# CADET_FLIGHT schema=2 count=2 rate_hz=18.2 scale=64
# INPUT bits 1=Z 2=/ 4=SPACE
FRAME XQ6 YQ6 VXQ6 VYQ6 BALLS READY TUBE EVENT CHARGE BORDER INPUT LEFT_HIT RIGHT_HIT LEFT_POWER RIGHT_POWER STEPS SCORE LATE STATE LEFT_SWEEP RIGHT_SWEEP
262 8086 11424 -1 45 3 0 0 0 96 0 2 0 1 0 1 1048 1000 4 0 0 3
263 11584 9024 0 0 2 1 1 3 0 0 2 0 1 0 1 1052 1000 4 0 0 2
"""


class FlightTests(unittest.TestCase):
    def test_legacy_and_new_formats(self):
        oldschema, oldcols, oldrows = parse_flight(OLD)
        self.assertEqual(oldschema, 1)
        self.assertEqual(len(oldrows), 2)
        oldnotes = "\n".join(summary_lines(oldrows, oldcols))
        self.assertIn("NOT IN THIS LEGACY LOG", oldnotes)
        self.assertIn("drain", oldnotes)
        schema, cols, rows = parse_flight(NEW)
        self.assertEqual(schema, 2)
        self.assertEqual(rows[-1]["RIGHT_POWER"], 1)
        notes = "\n".join(summary_lines(rows, cols))
        self.assertIn("keys=/", notes)
        self.assertIn("event=drain", notes)
        self.assertIn("1 total, 1 powered", notes)

    def test_reject_partial_or_corrupted_flight(self):
        for body in ("", "garbage", OLD.replace("263 11584", "263 ?"),
                     OLD.replace("263 11584", "261 11584"),
                     NEW.replace(" 0 2 0 1 0 1 1052", " 0 2 0 1 1052")):
            with self.subTest(body=body[:45]), self.assertRaises(ValueError):
                parse_flight(body)

    def test_summary_counter_semantics(self):
        out = parse_summary("PASS frames=288 input=6 balls=2 right_power=1\n")
        self.assertEqual(out["input"], 6)
        self.assertEqual(out["right_power"], 1)
        with self.assertRaises(ValueError):
            parse_summary("PASS hits=2 hits=3")
        with self.assertRaises(ValueError):
            parse_summary("FAIL frames=100")

    def test_end_to_end_with_emulator_startup_warnings(self):
        with tempfile.TemporaryDirectory() as root:
            folder = Path(root)
            (folder / "FLIGHT.TXT").write_text(NEW)
            (folder / "INPUT.TXT").write_text("PASS frames=288 input=6 balls=2\n")
            (folder / "TEST.LOG").write_text("STAGE: checks\nAll DOS core tests passed.\n")
            (folder / "CORE.LOG").write_text(
                "LOG: WARNING: DOS/V is only supported for VGA video cards.\n"
                "LOG: CPU warning: 8086 cpu type is experimental at this time\n"
                "LOG: MIDI: synth does not have soundfont\n"
            )
            output = summarize(folder)
            self.assertIn("keys ever seen: /+SPACE", output)
            self.assertIn("DOSBox startup warnings=2, fatal/error indicators=0", output)
            self.assertIn("all core tests passed", output)
            self.assertIn("FLIGHT.TXT: schema=2", output)


if __name__ == "__main__":
    unittest.main()
