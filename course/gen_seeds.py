import base64
from pathlib import Path

# 1x1 transparent PNG
SEED = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO2r3ZkAAAAASUVORK5CYII="
)

def main():
    out = Path(__file__).parent / "seed_corpus"
    out.mkdir(parents=True, exist_ok=True)
    for i in range(10):
        (out / f"seed_{i}.png").write_bytes(SEED)

if __name__ == "__main__":
    main()
