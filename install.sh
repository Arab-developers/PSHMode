rm -rif psh-mode ~/.psh-mode ~/../usr/bin/psh-mode &>/dev/null
rm -f psh-modeInstall &>/dev/null
cd

if command -v sudo >/dev/null; then
  sudo python3 -B psh-mode delete &>/dev/null
  sudo python3 -B psh-mode delete &>/dev/null
  if ! command -v wget >/dev/null; then
    sudo apt install wget zip unzip
    sudo apt update -y
    sudo apt upgrade -y
  fi
else
  python3 -B psh-mode delete &>/dev/null
  python3 -B psh-mode delete &>/dev/null
  if ! command -v wget >/dev/null; then
    pkg install wget zip unzip
    pkg update -y
    pkg upgrade -y
    termux-setup-storage
  fi
fi

rm -f main.zip
wget https://github.com/Arab-developers/psh-mode/archive/refs/heads/main.zip
unzip main.zip &>/dev/null
rm -f main.zip
mv -f psh-mode-main psh-mode
if command -v sudo >/dev/null; then
  sudo chmod +x psh-mode/bin/*
  sudo chmod -x psh-mode/bin/activate
else
  chmod +x psh-mode/bin/*
  chmod -x psh-mode/bin/activate
fi
if command -v sudo >/dev/null; then
  sudo python3 -B psh-mode add_shortcut
  python3 -B psh-mode install
else
  if ! [ -d "/sdcard/psh-mode/" ]; then
    mkdir "/sdcard/psh-mode/"
  fi
  python3 -B psh-mode add_shortcut
  python3 -B psh-mode install
fi

function psh-mode() {
  if [ $1 ]; then
    if [ $1 == "check" ]; then
      $HOME/.psh-mode/psh-mode/bin/psh-mode check
    elif [ $1 == "update" ]; then
      $HOME/.psh-mode/psh-mode/bin/psh-mode update
    elif [ $1 == "delete" ]; then
      $HOME/.psh-mode/psh-mode/bin/psh-mode delete
    else
      $HOME/.psh-mode/psh-mode/bin/psh-mode --help
    fi
  else
    if [ $VIRTUAL_ENV ]; then
      echo "psh-mode is running..."
    else
      source $HOME/.psh-mode/psh-mode/bin/activate
    fi
  fi
}

if command -v termux-reload-settings >/dev/null; then
  mkdir $HOME/.termux &>/dev/null
  if [ -f $HOME/.termux/font.ttf ]; then
    mv -f $HOME/.termux/font.ttf $HOME/.termux/.old_font.ttf
  fi
  cp -f $HOME/.psh-mode/psh-mode/share/fonts/DejaVu.ttf $HOME/.termux/font.ttf
  termux-reload-settings
fi