"""Create deterministic gzip companions for LittleFS, with no ESP compression cost."""
import gzip
from pathlib import Path

Import("env")
data_dir = Path(env.subst("$PROJECT_DATA_DIR"))
for source in data_dir.iterdir():
    if source.suffix not in {".html", ".css", ".js", ".svg"}:
        continue
    target = source.with_name(source.name + ".gz")
    compressed = gzip.compress(source.read_bytes(), compresslevel=9, mtime=0)
    if not target.exists() or target.read_bytes() != compressed:
        target.write_bytes(compressed)
