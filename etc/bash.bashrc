# This file is running before loading the system shell.
# You can put anything here.

# The VIRTUAL_ENV variable give you the psh-mode path.
# You can use it to run files in etc folder.


# psh-mode shortcuts
alias c="clear"
alias gc="git clone"
alias p3i="pip3 install"
alias p2i="pip2 install"
alias pi="pip install"
alias cdSD="cd /sdcard"
alias cdDO="cd /sdcard/Download"

# psh-mode prompt
PS1="\[\033[0;34m\]┌──\[\033[1;34m\](\[\033[1;31m\]PSH💀MODE\[\033[1;34m\])\[\033[0;34m\]-\[\033[1;34m\][\[\033[0m\]\W\[\033[1;34m\]]\n\[\033[0;34m\]└─\[\033[1;31m\]\$\[\033[0m\] "
# ┌──(PSH💀MODE)-[home]
# └─$

# psh-mode intro
# python3 $VIRTUAL_ENV/etc/intro.py