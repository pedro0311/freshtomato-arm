# **FreshTomato-ARM** #

**Forked off from Tomato-ARM by Shibby, builds compiled by pedro**

Forums about tomato:

PL: https://openlinksys.info/forum/

EN: https://www.linksysinfo.org/

This is my personal fork, If anyone wants to pick up my changes and merge them to your repository, feel free and go ahead. That's the reason Tomato is an open-source project.

**Source code**: https://bitbucket.org/pedro311/freshtomato-arm/commits/all

**Downloads**: https://mega.nz/#F!QywknIpa!5JwWNIfEwCOKXqXG0AOh4w

For the following **ARM** routers: **Asus** N18U, AC56S, AC56U/AC56R, AC68U(A1,A2,B1)/R/P, AC3200, **Netgear** R6250, R6300v2, R6400, R7000, R8000, **Linksys** EA6300v1/EA6400, EA6500v2, EA6700, EA6900, **Tenda** AC15, **Huawei** WS880, **Dlink** DIR868L, **Xiaomi** R1D.

Disclaimer: I am not responsible for any bricked routers, nor do I encourage other people to flash alternative firmwares on their routers. Use at your own risk!


**HOW TO COMPILE FRESHTOMATO on Debian 9.x/64bit**

1. Login as root

2. Update system:  
    apt-get update  
    apt-get dist-upgrade  

3. Install basic packages:  
    apt-get install build-essential net-tools  

4. NOT NECESSARY (depends if sys is on vmware); install vmware-tools:  
    mkdir /mnt/cd  
    mount /dev/cdrom /mnt/cd  
    unpack  
    ./vmware-install.pl  

5. Set proper date/time:  
    dpkg-reconfigure tzdata  

6. Add your <username> to sudo group:  
    apt-get install sudo  
    adduser <username> sudo  
    reboot  

7. Login as <username>, install base packages with all dependencies:  
    sudo apt-get install autoconf m4 bison flex g++ libtool sqlite gcc binutils patch bzip2 make gettext unzip zlib1g-dev libc6 gperf automake groff  
    sudo apt-get install lib32stdc++6 libncurses5 libncurses5-dev gawk gitk zlib1g-dev autopoint shtool autogen mtd-utils gcc-multilib gconf-editor lib32z1-dev pkg-config libssl-dev automake1.11  
    sudo apt-get install libmnl-dev libxml2-dev intltool libglib2.0-dev libstdc++5 texinfo dos2unix xsltproc libnfnetlink0 libcurl4-openssl-dev libgtk2.0-dev libnotify-dev libevent-dev mc git  
    sudo apt-get install re2c texlive libelf1  
    sudo apt-get install linux-headers-`uname -r`  

8. Remove libicu-dev if it's installed, it stopped PHP compilation:  
    sudo apt-get remove libicu-dev  

9. Install i386 elf1 packages:  
    sudo dpkg --add-architecture i386  
    sudo apt-get update  
    sudo apt-get install libelf1:i386 libelf-dev:i386  

10. Clone/download repository:  
    git clone https://bitbucket.org/pedro311/freshtomato-arm.git <chosen-subdir>  

11. Edit profile file:  
    PATH="$PATH:/home/<username>/<chosen-subdir>/release/src-rt-6.x.4708/toolchains/hndtools-arm-linux-2.6.36-uclibc-4.5.3/bin"  
    PATH="$PATH:/sbin"  

12. Reboot system  

13. Add your email to git config:  
    git config --global user.email "<email-address>"  
   or  
    git config user.email "<email-address>"  
   for a single repo  

14. Add your username to git config:  
    git config --global user.name <name>  
  
**You're ready**
