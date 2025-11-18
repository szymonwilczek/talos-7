#!/bin/bash

# otworz terminal uzywajac xdotool do symulacji super+enter
xdotool key super+Return

# poczekaj na uruchomienie terminala
sleep 0.5

# uruchomienie firefoxa w tle
firefox &

# poczekaj az firefox sie uruchomi
sleep 2

# znajdz id okna firefoxa
FIREFOX_WINDOW=$(xdotool search --class "firefox" | tail -1)

# aktywuj okno firefoxa
xdotool windowactivate $FIREFOX_WINDOW

# poczekaj chwile na aktywacje
sleep 0.3

# przenies firefox na lewy monitor
xdotool key super+Shift+Left

# poczekaj na przesuniecie okna
sleep 0.3

# znajdz id okna terminala
TERMINAL_WINDOW=$(xdotool search --class "gnome-terminal" | tail -1)

# przywroc focus do terminala
if [ -n "$TERMINAL_WINDOW" ]; then
    xdotool windowactivate $TERMINAL_WINDOW
fi
