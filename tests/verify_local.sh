#!/usr/bin/env bash
set -euo pipefail

EMCC="/home/stevemakesgames/.gemini/antigravity-cli/brain/20ce9de4-efa4-40c7-9765-4625149cbcd2/scratch/emsdk/upstream/emscripten/emcc"
NODE="/home/stevemakesgames/.gemini/antigravity-cli/brain/20ce9de4-efa4-40c7-9765-4625149cbcd2/scratch/emsdk/node/24.19.0_64bit/bin/node"
SMOKE_HARNESS="/home/stevemakesgames/.gemini/antigravity-cli/brain/20ce9de4-efa4-40c7-9765-4625149cbcd2/scratch/run_web_smoke.js"

echo "[1/3] Compiling & running Unit Test Suite..."
"$EMCC" -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
    src/core/grind_core.c src/app/grind_input.c tests/unit/test_main.c \
    -lm -sSTACK_OVERFLOW_CHECK=2 -o /tmp/grind_unit_tests.js
"$NODE" /tmp/grind_unit_tests.js

echo "[2/3] Compiling & running Automated Game Progression Suite..."
"$EMCC" -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
    src/core/grind_core.c src/app/grind_input.c tests/game/test_game_progression.c \
    -lm -sSTACK_OVERFLOW_CHECK=2 -o /tmp/grind_game_tests.js
"$NODE" /tmp/grind_game_tests.js

echo "[3/3] Compiling & running WebGL 2.0 Application Smoke Test (with Stack Overflow & Assertion checks)..."
"$EMCC" -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude \
    src/core/grind_core.c src/app/grind_input.c src/app/grind_renderer.c src/main.c \
    -sUSE_SDL=2 -sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
    -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=1048576 \
    -sSTACK_OVERFLOW_CHECK=2 -sASSERTIONS=2 \
    -o /tmp/grind_web_smoke.js
"$NODE" "$SMOKE_HARNESS" /tmp/grind_web_smoke.js

echo "=== ALL LOCAL VERIFICATION SUITES PASSED ==="
