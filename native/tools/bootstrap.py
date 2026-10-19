"""Fetch the pinned dependencies: Aurora and the recovered game sources.

Game code is not part of this repository. It is fetched into build/native-deps
and linked as ./src and ./extern (both gitignored), where CMake expects them.
"""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
deps = root / 'build/native-deps'


def git(*args, cwd=None):
    subprocess.run(['git', *args], cwd=cwd, check=True)


def head(checkout):
    return subprocess.check_output(['git', '-C', str(checkout), 'rev-parse', 'HEAD'], text=True).strip()


# AdamGoodApp/aurora `melee-web`: upstream 749d6ee plus the Emscripten/WebGPU browser backend.
aurora = deps / 'aurora'
aurora_revision = '520a3f78142af88cec26dd03baad50e16778203f'
if not aurora.exists():
    deps.mkdir(parents=True, exist_ok=True)
    git('clone', 'https://github.com/AdamGoodApp/aurora.git', str(aurora))
    git('-C', str(aurora), 'checkout', '--detach', aurora_revision)
if head(aurora) != aurora_revision:
    raise SystemExit(f'Aurora checkout is {head(aurora)}; expected {aurora_revision}. Existing checkout left untouched.')
print(f'Aurora source verified at {aurora_revision}')

# AdamGoodApp/melee-native `web`: doldecomp/melee plus the native/wasm32 fixes.
game = deps / 'game'
game_revision = 'd3b87adc829d97f6ccc3e770c0226f5191242efd'
if not game.exists():
    game.mkdir(parents=True)
    git('init', '-q', cwd=game)
    git('remote', 'add', 'origin', 'https://github.com/AdamGoodApp/melee-native.git', cwd=game)
    git('sparse-checkout', 'set', '--no-cone', '/src/', '/extern/', cwd=game)
    git('fetch', '--depth', '1', '--filter=blob:none', 'origin', game_revision, cwd=game)
    git('checkout', '-q', '--detach', 'FETCH_HEAD', cwd=game)
if head(game) != game_revision:
    raise SystemExit(f'Game source checkout is {head(game)}; expected {game_revision}. Existing checkout left untouched.')
for name in ('src', 'extern'):
    link = root / name
    if link.is_symlink() and link.resolve() == (game / name).resolve():
        continue
    if link.exists() or link.is_symlink():
        raise SystemExit(f'{link} exists and is not the fetched game source; remove it and rerun.')
    link.symlink_to(game / name, target_is_directory=True)
print(f'Game source verified at {game_revision}')
