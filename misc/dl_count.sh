#!/bin/bash

curl -s https://api.github.com/repos/samsonwang/LaunchyQt/releases | egrep '"name"|"download_count"' | head -16
