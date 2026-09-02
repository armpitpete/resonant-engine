@echo off
setlocal
where py >nul 2>&1
if %errorlevel%==0 (
  set PY=py
) else (
  set PY=python
)
start "" http://127.0.0.1:8000/
%PY% -m http.server 8000 --bind 127.0.0.1
