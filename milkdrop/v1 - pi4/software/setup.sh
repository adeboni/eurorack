sudo apt update
sudo apt upgrade
sudo apt install raspberrypi-kernel-headers build-essential cmake libgl1-mesa-dev mesa-common-dev libglm-dev mesa-utils flex bison openssl libssl-dev git libsdl2-dev libfreetype-dev cmake xautomation pulseaudio

cd ~
wget https://github.com/projectM-visualizer/projectm/archive/refs/tags/v4.0.0.tar.gz
tar xf v4.0.0.tar.gz
cd ~/projectm-4.0.0/
mkdir build
cd build
cmake -DENABLE_GLES=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
cmake --build . --parallel && sudo cmake --build . --target install

cd ~
wget https://pocoproject.org/releases/poco-1.12.5/poco-1.12.5-all.tar.bz2
tar -xjf poco-1.12.5-all.tar.bz2
cd poco-1.12.5-all/
mkdir cmake-build
cd cmake-build
cmake ..
cmake --build . --config Release
sudo cmake --build . --target install
sudo cp /usr/local/lib/libPoco* /usr/lib/

cd ~
git clone https://github.com/kholbrook1303/frontend-sdl2.git
cd frontend-sdl2/
git submodule init
git submodule update
mkdir cmake-build
cmake -S . -B cmake-build -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build --config Release
cd cmake-build
make
sudo mkdir /opt/ProjectMSDL
sudo cp -r ~/frontend-sdl2/cmake-build/src/* /opt/ProjectMSDL/
sudo chmod 777 -R /opt/ProjectMSDL

cd ~
git clone https://github.com/mickabrig7/projectM-presets-rpi5.git

# nano /opt/ProjectMSDL/projectMSDL.properties
# window.fullscreen = true
# projectM.meshX = 64
# projectM.meshY = 32
# update preset config path

# sudo nano /etc/environment
# MESA_GL_VERSION_OVERRIDE=4.5

# sudo nano /boot/firmware/cmdline.txt
# add video=HDMI-A-1:1280x720M@60 video=HDMI-A-2:1280x720M@60

cd ~
git clone https://github.com/kholbrook1303/RPI5-Bookworm-ProjectM-Audio-Receiver.git
cp -r ~/RPI5-Bookworm-ProjectM-Audio-Receiver/* /opt/ProjectMSDL/
cd /opt/ProjectMSDL/
python3 -m venv env
/opt/ProjectMSDL/env/bin/python3 -m pip install -r requirements.txt

# nano /opt/ProjectMSDL/projectMAR.conf
# update from pactl list sources/sinks short

cd ~
mkdir codec
cd codec
wget https://raw.githubusercontent.com/raspberrypi/linux/rpi-6.1.y/sound/soc/codecs/cs4270.c
make
sudo cp cs4270.ko /lib/modules/$(uname -r)/cs4270.ko
sudo depmod
sudo modprobe cs4270

wget https://raw.githubusercontent.com/monome/linux/norns-5.10.y/arch/arm/boot/dts/overlays/monome-snd-4270-overlay.dts
dtc -I dts -O dtb monome-snd-4270-overlay.dts -o monome-snd.dtbo
sudo cp monome-snd.dtbo /boot/overlays/monome-snd.dtbo
# add to /boot/firmware/config.txt
# enable i2s and i2c
# dtoverlay=monome-snd
# dtoverlay=gpio-key,gpio=12,active_low=1,gpio_pull=up,keycode=19

# sudo nano /etc/rc.local
# add /opt/ProjectMSDL/env/bin/python3 /opt/ProjectMSDL/projectMAR.py

# sudo raspi-config, then select Advanced Options -> Audio Config -> Pipewire
#                                System Options -> Boot / Auto Logon -> Console Auto Logon
#                                Performance -> Overlay File System