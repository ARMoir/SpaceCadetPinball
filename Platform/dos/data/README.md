# Converted Space Cadet resources

These files were generated from the Windows `PINBALL.DAT` supplied by the
repository owner for this port. The owner authorized committing the assets
needed for the playable prototype. The original archive and Windows EXE are
not included.

* `TABLE.TGA`: 32768-byte raw Tandy mode 09h bank image (not Targa).
* `TABLE.DAT`: SCP1 geometry and five moving sprites.
* `PREVIEW.png`: preview of the converted static background.
* `IMPORT.json`: source SHA-256, imported/omitted objects and simplifications.

Regenerate from the repository root:

```
python Platform/dos/convert.py /path/to/PINBALL.DAT --output Platform/dos/data
```

The artwork and resource data originate from Space Cadet, developed by
Cinematronics and distributed with Microsoft Windows. They are not newly
MIT-licensed artwork; the repository's code license does not relicense these
original game resources. The converter and DOS implementation are project code.
