# python ./web2c.py
#!/usr/bin/env python3
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parent
DIST_DIR = ROOT / 'web' / 'dist'
OUT_DIR = ROOT / 'generated'
FILES = [
    ('index.html', 'index_html', 'text/html; charset=utf-8'),
    ('main.css', 'main_css', 'text/css; charset=utf-8'),
    ('app.js', 'app_js', 'application/javascript; charset=utf-8'),
]
def to_c_array(data: bytes, width: int = 12) -> str:
    items = [f'0x{b:02X}' for b in data]
    lines = []
    for i in range(0, len(items), width):
        lines.append('    ' + ', '.join(items[i:i + width]))
    return ',\n'.join(lines)
def make_guard(symbol: str) -> str:
    return re.sub(r'[^A-Za-z0-9]+', '_', symbol).upper() + '_H'
def write_header(var_name: str, content_type: str) -> str:
    guard = make_guard(var_name)
    return f'''#ifndef {guard}
#define {guard}
#include <stdint.h>
#ifdef __cplusplus
extern "C" {{
#endif
extern const uint8_t {var_name}[];
extern const uint32_t {var_name}_len;
extern const char {var_name}_content_type[];
#ifdef __cplusplus
}}
#endif
#endif
'''
def write_source(var_name: str, header_name: str, content_type: str, data: bytes) -> str:
    body = to_c_array(data)
    return f'''#include "{header_name}"
const uint8_t {var_name}[] = {{
{body}
}};
const uint32_t {var_name}_len = {len(data)}u;
const char {var_name}_content_type[] = "{content_type}";
'''
def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for src_name, var_name, content_type in FILES:
        src_path = DIST_DIR / src_name
        if not src_path.exists():
            raise FileNotFoundError(f'missing dist file: {src_path}')
        data = src_path.read_bytes()
        header_path = OUT_DIR / f'{var_name}.h'
        source_path = OUT_DIR / f'{var_name}.c'
        header_path.write_text(write_header(var_name, content_type), encoding='utf-8')
        source_path.write_text(write_source(var_name, header_path.name, content_type, data), encoding='utf-8')
        print(f'generated: {source_path.name}, {header_path.name}')
if __name__ == '__main__':
    main()
