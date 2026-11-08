import tempfile
import unittest
from pathlib import Path
from package import package, RUNTIME_FILES


class PackageTests(unittest.TestCase):
    def test_runtime_only_and_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            source, destination = Path(tmp)/"source", Path(tmp)/"runtime"
            source.mkdir()
            for name, data in {"CADET.EXE": b"MZtest", "TABLE.DAT": b"SCP2test",
                               "TABLE.TGA": bytes(32768), "TABLE3D.TGA": bytes(32768),
                               "SCORES.DAT": b"private", "TEST.EXE": b"test",
                               "SMOKE.TXT": b"logs"}.items():
                (source/name).write_bytes(data)
            package(source, destination)
            self.assertEqual(set(p.name for p in destination.iterdir()), set(RUNTIME_FILES))
            for name in RUNTIME_FILES:
                self.assertEqual((source/name).read_bytes(), (destination/name).read_bytes())
            (source/"TABLE3D.TGA").write_bytes(b"truncated")
            with self.assertRaises(ValueError):
                package(source, Path(tmp)/"invalid")
            self.assertFalse((Path(tmp)/"invalid").exists())
            (source/"TABLE3D.TGA").unlink()
            with self.assertRaises(FileNotFoundError):
                package(source, Path(tmp)/"missing")


if __name__ == "__main__":
    unittest.main()
