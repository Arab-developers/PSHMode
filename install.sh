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
  echo -e "Start installing for ( \033[1;32m$PLATFORM\033[0m )"
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
else
  echo "# No support for '$PLATFORM'!"
fi

# Remove variables from the global namespace.
unset PLATFORM PSHMODE_PACKAGES LOG_FILE
unset -f download_PSHMode

# Add PSHMode command line.
source "$HOME"/.PSHMode/PSHMode.shortcut
