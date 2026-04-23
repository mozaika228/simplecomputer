#!/usr/bin/env bash
set -euo pipefail

echo "TERM=${TERM:-unknown}"
echo "Escape capabilities from infocmp:"
infocmp -1 2>/dev/null | grep -E '^(clear|cup|setaf|setab|op|el|civis|cnorm)=' || true

# Clear screen and move cursor home.
echo -e "\e[2J\e[H"

# Name in red on black at row 5, col 10.
echo -e "\e[5;10H\e[31;40mDanila"

# Group in green on white at row 6, col 8.
echo -e "\e[6;8H\e[32;47mIT-projects"

# Move cursor to row 10, col 1 and reset colors.
echo -e "\e[10;1H\e[0m"
