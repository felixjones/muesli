#!/usr/bin/env python3
# AI Generated file! TODO: Replace with human code
import re
import sys
import os
from datetime import datetime

# Pattern that allows:
# - Single year: Copyright (c) 2026, Felix Jones
# - Year range: Copyright (c) 2026-2027, Felix Jones
# - With contributors: Copyright (c) 2026, Felix Jones, and contributors
# - With multiple names: Copyright (c) 2026-2027, Felix Jones, Jim Bob, and contributors
LICENSE_PATTERN = re.compile(
    r"^/\*\s*\n"
    r" \* BSD 3-Clause License\n"
    r" \* Copyright \(c\) (?P<years>\d{4}(?:-\d{4})?), (?P<owners>[^\n]+?)\n"
    r" \* See LICENSE file for details\.\n"
    r" \*/\n",
    re.MULTILINE
)

EXCLUDED_EXTS = {'.md', '.txt'}
INCLUDE_DIRS = ['include']
CURRENT_YEAR = datetime.now().year

def validate_copyright_format(match):
    """Validate that copyright years and owners follow the rules."""
    years_str = match.group('years')
    owners_str = match.group('owners')

    # Parse years
    if '-' in years_str:
        try:
            start_year, end_year = map(int, years_str.split('-'))
        except ValueError:
            return False, "Invalid year range format"

        # Check: start <= end
        if start_year > end_year:
            return False, f"Start year {start_year} is after end year {end_year}"

        # Check: both years in valid range [2026, current_year]
        if start_year < 2026 or end_year < 2026:
            return False, f"Years must be >= 2026"
        if start_year > CURRENT_YEAR or end_year > CURRENT_YEAR:
            return False, f"Years must be <= {CURRENT_YEAR}"
    else:
        year = int(years_str)
        if year < 2026 or year > CURRENT_YEAR:
            return False, f"Year {year} must be between 2026 and {CURRENT_YEAR}"

    # Check owners format: must have at least one name
    if not owners_str.strip():
        return False, "No owner names provided"

    return True, None

def file_has_license_header(path):
    try:
        with open(path, 'r', encoding='utf-8') as f:
            content = f.read()
        m = LICENSE_PATTERN.match(content)
        if not m:
            return False

        # Validate the copyright format
        is_valid, error_msg = validate_copyright_format(m)
        if not is_valid:
            print(f"  Warning in {os.path.relpath(path)}: {error_msg}")
            return False

        return True
    except Exception as e:
        print(f"  Error reading {path}: {e}")
        return False

def collect_headers(root):
    files = []
    for base in INCLUDE_DIRS:
        dirpath = os.path.join(root, base)
        for r, _, names in os.walk(dirpath):
            for n in names:
                ext = os.path.splitext(n)[1]
                if ext in EXCLUDED_EXTS:
                    continue
                path = os.path.join(r, n)
                files.append(path)
    return files

def main():
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    headers = collect_headers(root)
    missing = []
    for h in headers:
        if not file_has_license_header(h):
            missing.append(h)
    if missing:
        print('License header missing or malformed in the following files:')
        for m in sorted(missing):
            print(f' - {os.path.relpath(m, root)}')
        print('\nExpected header format:')
        print('''/*\n * BSD 3-Clause License\n * Copyright (c) <START_YEAR>[-END_YEAR], <NAME>[, <NAME>][, and contributors]\n * See LICENSE file for details.\n */\n''')
        print(f'\nYear constraints: {2026} <= year <= {CURRENT_YEAR}')
        print('For year ranges: START_YEAR <= END_YEAR')
        return 1
    print(f'✓ All {len(headers)} headers have valid license headers.')
    return 0

if __name__ == '__main__':
    sys.exit(main())
