from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent
SRC = ROOT / "web" / "src"
DIST = ROOT / "web" / "dist"

CSS_FILES = [
    SRC / "css" / "base.css",
    SRC / "css" / "layout.css",
    SRC / "css" / "components.css",
]

JS_FILES = [
    SRC / "js" / "state.js",
    SRC / "js" / "utils.js",
    SRC / "js" / "ui" / "elements.js",
    SRC / "js" / "ui" / "log.js",
    SRC / "js" / "ui" / "ipInput.js",
    SRC / "js" / "api.js",
    SRC / "js" / "features" / "status.js",
    SRC / "js" / "features" / "network.js",
    SRC / "js" / "features" / "boot.js",
    SRC / "js" / "features" / "system.js",
    SRC / "js" / "features" / "download.js",
    SRC / "js" / "features" / "timeSync.js",
    SRC / "js" / "features" / "fs.js",
    SRC / "js" / "app.js",
]

def ensure_dist():
    DIST.mkdir(parents=True, exist_ok=True)

def build_css():
    parts = []
    for path in CSS_FILES:
        text = path.read_text(encoding="utf-8")
        parts.append(f"/* ===== {path.name} ===== */\n{text.strip()}\n")
    out = "\n".join(parts)
    (DIST / "main.css").write_text(out, encoding="utf-8")

def strip_es_module_syntax(js_text: str) -> str:
    js_text = re.sub(r'^\s*import\s+.*?;\s*$', '', js_text, flags=re.MULTILINE)
    js_text = re.sub(r'^\s*export\s+', '', js_text, flags=re.MULTILINE)
    return js_text

def build_js():
    parts = []
    for path in JS_FILES:
        text = path.read_text(encoding="utf-8")
        text = strip_es_module_syntax(text)
        parts.append(f"// ===== {path.name} =====\n{text.strip()}\n")
    out = "\n\n".join(parts)
    (DIST / "app.js").write_text(out, encoding="utf-8")

def build_html():
    html = (SRC / "index.html").read_text(encoding="utf-8")
    html = re.sub(
        r'<link[^>]+href="\.\/css\/base\.css"[^>]*>\s*',
        '',
        html,
        flags=re.MULTILINE
    )
    html = re.sub(
        r'<link[^>]+href="\.\/css\/layout\.css"[^>]*>\s*',
        '',
        html,
        flags=re.MULTILINE
    )
    html = re.sub(
        r'<link[^>]+href="\.\/css\/components\.css"[^>]*>\s*',
        '',
        html,
        flags=re.MULTILINE
    )
    html = re.sub(
        r'<script[^>]+type="module"[^>]+src="\.\/js\/app\.js"[^>]*>\s*</script>',
        '',
        html,
        flags=re.MULTILINE
    )

    html = html.replace("</head>", '  <link rel="stylesheet" href="/main.css">\n</head>')
    html = html.replace("</body>", '  <script src="/app.js"></script>\n</body>')

    (DIST / "index.html").write_text(html, encoding="utf-8")

def main():
    ensure_dist()
    build_css()
    build_js()
    build_html()
    print("Build completed:")
    print(f"  {DIST / 'index.html'}")
    print(f"  {DIST / 'main.css'}")
    print(f"  {DIST / 'app.js'}")

if __name__ == "__main__":
    main()
