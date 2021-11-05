rm -rif HackerMode ~/.HackerMode ~/../usr/bin/HackerMode &>/dev/null
if command -v sudo >/dev/null; then
  sudo python3 -B HackerMode delete &>/dev/null
  if ! command -v wget >/dev/null; then
    sudo apt install wget
  fi
else
  python3 -B HackerMode delete &>/dev/null
  if ! command -v wget >/dev/null; then
    pkg install wget
  fi
fi

rm -f future.zip
wget https://github.com/Arab-developers/HackerMode/archive/refs/heads/future.zip
unzip future.zip &>/dev/null
rm -f future.zip
mv -f HackerMode-future HackerMode
if command -v sudo >/dev/null; then
  sudo chmod +x HackerMode/bin/*
  sudo chmod -x HackerMode/bin/activate
else
  chmod +x HackerMode/bin/*
  chmod -x HackerMode/bin/activate
fi
if command -v sudo >/dev/null; then
  sudo python3 -B HackerMode add_shortcut
  python3 -B HackerMode install
else
  if ! [ -d "/sdcard/HackerMode/" ]; then
    mkdir "/sdcard/HackerMode/"
  fi
  python3 -B HackerMode add_shortcut
  python3 -B HackerMode install
fi

function HackerMode() {
  if [ $1 ]; then
    if [ $1 == "check" ]; then
      $HOME/.HackerMode/HackerMode/bin/HackerMode check
    elif [ $1 == "update" ]; then
      $HOME/.HackerMode/HackerMode/bin/HackerMode update
    elif [ $1 == "delete" ]; then
      $HOME/.HackerMode/HackerMode/bin/HackerMode delete
    else
      $HOME/.HackerMode/HackerMode/bin/HackerMode --help
    fi
  else
    if [ $VIRTUAL_ENV ]; then
      echo "HackerMode is running..."
    else
      source $HOME/.HackerMode/HackerMode/bin/activate
    fi
  fi
}

if command -v termux-reload-settings >/dev/null; then
  mkdir $HOME/.termux &>/dev/null
  if [ -f $HOME/.termux/font.ttf ]; then
    mv -f $HOME/.termux/font.ttf $HOME/.termux/.old_font.ttf
  fi
  cp -f $HOME/.HackerMode/HackerMode/share/fonts/DejaVu.ttf $HOME/.termux/font.ttf
  termux-reload-settings
fi

rm -f HackerModeInstall
