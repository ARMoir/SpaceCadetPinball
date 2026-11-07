import unittest
from run_tests import validate_smoke


class SmokeValidationTests(unittest.TestCase):
    report = ('PASS frames=360 steps=1440 launches=1 full_launches=1 '
              'hits=1 score=500 slow_frames=4\n'
              '# art=3d static_baked=1 moving_ball_shadow=0')

    def test_completed_gameplay(self):
        self.assertEqual(validate_smoke(self.report, original=True)['score'], 500)

    def test_incomplete_or_old_reports(self):
        for report in ('', self.report.replace('PASS', 'FAIL'),
                       self.report.replace(' full_launches=1', ''),
                       self.report.replace('frames=360', 'frames=359'),
                       self.report.replace('steps=1440', 'steps=1400')):
            with self.subTest(report=report), self.assertRaises(RuntimeError):
                validate_smoke(report, original=True)

    def test_no_full_launch(self):
        for report in (self.report.replace('full_launches=1', 'full_launches=0'),
                       self.report.replace('launches=1 full', 'launches=0 full')):
            with self.subTest(report=report), self.assertRaises(RuntimeError):
                validate_smoke(report, original=True)

    def test_ball_sprite_has_no_dynamic_overlay(self):
        for report in (self.report.replace('moving_ball_shadow=0', 'moving_ball_shadow=1'),
                       self.report.replace(' moving_ball_shadow=0', '')):
            with self.subTest(report=report), self.assertRaises(RuntimeError):
                validate_smoke(report, original=True)

    def test_no_scoring_progress(self):
        for report in (self.report.replace('hits=1', 'hits=0'),
                       self.report.replace('score=500', 'score=0')):
            with self.subTest(report=report), self.assertRaises(RuntimeError):
                validate_smoke(report, original=True)

    def test_diagnostic_table_has_no_original_score_requirement(self):
        validate_smoke(self.report.replace('hits=1 score=500', 'hits=0 score=0'))


if __name__ == '__main__':
    unittest.main()
