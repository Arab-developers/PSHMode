<<<<<<< Updated upstream
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
=======
# PSHMode logs
LOG_FILE=".PSHMode-install.log"
echo "" >$LOG_FILE

# Get the platform.
PLATFORM=$(python3 -c "import sys, os;print('win' if sys.platform in ('win32', 'cygwin') else 'macosx' if sys.platform == 'darwin' else 'termux' if os.environ.get('PREFIX') != None else 'linux' if sys.platform.startswith('linux') or sys.platform.startswith('freebsd') else 'unknown')")

# Remove old version from the tool.
python3 -c 'import subprocess;subprocess.run(["bash", "-i", "-c", "HackerMode delete"], stdout=subprocess.PIPE, text=True, input="y")' &> /dev/null
rm -rif HackerMode ~/.HackerMode ~/../usr/bin/HackerMode &>/dev/null
rm -f HackerModeInstall &>/dev/null
rm -rif PSHMode ~/.PSHMode ~/../usr/bin/PSHMode &>/dev/null
rm -f PSHMode.install &>/dev/null

# To install before setup the tool.
PSHMODE_PACKAGES=(
  wget
  git
  unzip
  zip
)

# Download PSHMode and move it to home.
function download_PSHMode() {
  cd "$HOME"
  rm -f main.zip
  wget https://github.com/Arab-developers/PSHMode/archive/refs/heads/main.zip &>>$LOG_FILE
  unzip main.zip &>>$LOG_FILE
  rm -f main.zip
  mv -f PSHMode-main .PSHMode
}

if [[ $PLATFORM != "unknown" ]]; then
  echo -e "\n\nStart installing for ( \033[1;32m$PLATFORM\033[0m )"
fi

# Linux installation...
if [[ $PLATFORM == "linux" ]]; then
  # Install packages...
  sudo apt update -y
  sudo apt install python3 -y
  sudo apt install python3-pip -y
  for PKG in ${PSHMODE_PACKAGES[*]}; do
    sudo apt install "$PKG" -y
  done

  # Download the tool.
  download_PSHMode
  sudo python3 -B .PSHMode add_shortcut
  python3 -B .PSHMode install

# Termux installation...
elif [[ $PLATFORM == "termux" ]]; then
  # Install packages...
  pkg update -y
  pkg install python -y
  for PKG in ${PSHMODE_PACKAGES[*]}; do
    pkg install "$PKG" -y
  done
  pkg install zsh -y

  # Get sdcard permission.
  ls /sdcard &>>$LOG_FILE || termux-setup-storage

  # Download the tool.
  download_PSHMode
  python3 -B .PSHMode add_shortcut
  mkdir /sdcard/PSHMode &>>$LOG_FILE

  # Add the font
  if ! cmp --silent -- ".PSHMode/share/fonts/DejaVu.ttf" "~/.termux/font.ttf"; then
    cp .PSHMode/share/fonts/DejaVu.ttf ~/.termux/font.ttf
    termux-reload-settings
  fi

  # Start the installation
  python3 -B .PSHMode install

fi

# Remove variables from the global namespace.
unset PLATFORM PSHMODE_PACKAGES LOG_FILE
unset -f download_PSHMode

# Add PSHMode command line.
source "$HOME"/.PSHMode/PSHMode.shortcut
>>>>>>> Stashed changes
