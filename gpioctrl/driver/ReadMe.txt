-Basiert auf chardev Template
-Initialisiert alle GPIOs des Raspberry Pi4 als Ausgänge
-Die Offsets von den GPIOs koennen im File /sys/kernel/debug/gpio
 auf dem Raspberry Pi4 eingesehen werden
-Alle GPIOs können einzeln auf high oder low gesetzt werden
-In ~/.bashrc "export ARCH=arm64" und "export CROSS_COMPILE=aarch64-linux-gnu-"
-Bauen mit "make KDIR=/home/nicolai/Develop/Raspbian/linux"
-.ko Modul auf Raspberry Pi kopieren z.B. mit 
  "scp name.ko nicolai@192.168.2.50:/home/nicolai/temp"
-Modul installieren mit "sudo insmod name.ko"
-Installierte Module anschauen mit "lsmod"
-Installierte Devices anschauen mit "ls /dev"
-Lesen z.B. mit "sudo cat /dev/name"
-Schreiben z.B. mit "echo "Hi" | sudo tee /dev/name"
-Modul entfernen mit "sudo rmmod name"
-Status Nachrichten anschauen mit z.B. "dmesg | tail -n 20"