# PSHMode

<p>
<a href=""><img src="https://img.shields.io/github/repo-size/Arab-developers/PSHMode?label=tool size"></a>
</p>

- [Install](#install)
- [how to use?](#docs)

### Coded by PSH-TEAM

- [telegram](https://t.me/psh_team)
- [youtube](https://www.youtube.com/channel/UCRFNcuHk3I_1g6PBaBxj9qQ)

___

### Supported Operating Systems:

- kali
- ubuntu
- termux
- ish shell

<br>
<div id="install"></div>

### Installation:

**termux**

copy those commands and paste them on your terminal.

```bash
pkg install python zsh -y
```

```bash
chsh -s zsh
```
```bash
exit
```
```bash
curl https://raw.githubusercontent.com/Arab-developers/PSHMode/main/install.sh > PSHMode.install 2> .PSHMode-install.log && source PSHMode.install
```

then download the app to activate the tool after install.
<br>link: <a href="https://github.com/Arab-developers/HackerMode-Apk">PSHMode-APK</a>
___
**ish shell**

copy those commands and paste them on your terminal.

```bash
apk update
```
```bash
apk add python3 zsh curl shadow
```
```bash
chsh -s $(which zsh)
```
if you don't know what is the password you can write this command `passwd` to set new password or update it.
```bash
exit
```
```bash
curl https://raw.githubusercontent.com/Arab-developers/PSHMode/main/install.sh > PSHMode.install 2> .PSHMode-install.log && source PSHMode.install
```
___
**linux**

copy those commands and paste them on your terminal.

```bash
sudo apt install python3 zsh -y
```

```bash
chsh -s $(which zsh)
```
```bash
exit
```
```bash
curl https://raw.githubusercontent.com/Arab-developers/PSHMode/main/install.sh > PSHMode.install 2> .PSHMode-install.log && source PSHMode.install
```

<br>
<div id="docs"></div>

### How to use?

```shell
# to run the tool
$ PSHMode

# to check PSHMode packages
$ PSHMode check

# to update PSHMode and packages
$ PSHMode update

# to delete the tool.
$ PSHMode delete
```