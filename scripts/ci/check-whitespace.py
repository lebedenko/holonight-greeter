#!/usr/bin/env python3
"""Check current snapshot changes against HEAD, including non-ignored new inputs."""
import subprocess
import sys


def check(baseline, source):
    result = subprocess.run(['git', 'diff', '--no-index', '--check', str(baseline), str(source)])
    # --no-index returns 1 for clean differences; 3 includes whitespace errors.
    return 0 if result.returncode in (0, 1) else result.returncode


if __name__ == '__main__':
    sys.exit(check(sys.argv[1], sys.argv[2]))
