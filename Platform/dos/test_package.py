import tempfile
import unittest
from pathlib import Path
from package import package, RUNTIME_FILES
from build_menu_art import decode_source


class PackageTests(unittest.TestCase):
    def test_runtime_only_and_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            source, destination = Path(tmp)/"source", Path(tmp)/"runtime"
            source.mkdir()
            for name, data in {"CADET.EXE": b"MZtest", "TABLE.DAT": (Path(__file__).parent/"data/TABLE.DAT").read_bytes(),
                               "TABLE.TGA": bytes(32768), "TABLE3D.TGA": bytes(32768),
                               "VGA.DAT": (Path(__file__).parent/"data/VGA.DAT").read_bytes(),
                               "MENU.DAT": b"SCM1" + decode_source(),
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
            (source/"MENU.DAT").write_bytes(b"SCM1" + b"invalid")
            with self.assertRaises(ValueError):
                package(source, Path(tmp)/"invalid_menu")
            self.assertFalse((Path(tmp)/"invalid_menu").exists())
            with self.assertRaises(FileNotFoundError):
                package(source, Path(tmp)/"missing")


if __name__ == "__main__":
    unittest.main()
