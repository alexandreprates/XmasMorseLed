"""Embed the exact standalone page in the firmware; no filesystem upload needed."""
from pathlib import Path

Import("env")  # noqa: F821 -- PlatformIO/SCons provides the environment.
root = Path(env.subst("$PROJECT_DIR"))
generated = Path(env.subst("$BUILD_DIR")) / "generated"
generated.mkdir(parents=True, exist_ok=True)
page = (root / "web" / "index.html").read_text(encoding="utf-8")
assert ')XMORSE"' not in page
content = '#pragma once\n#include <Arduino.h>\nstatic const char WEB_PAGE[] PROGMEM = R"XMORSE(' + page + ')XMORSE";\n'
target = generated / "WebPage.h"
if not target.exists() or target.read_text(encoding="utf-8") != content:
    target.write_text(content, encoding="utf-8")
env.Append(CPPPATH=[str(generated)])
