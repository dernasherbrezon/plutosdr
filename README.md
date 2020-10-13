# plutosdr

This program will configure fir filter for plutosdr and start reading the data. With decimating fir filter it is possible to set sample rate up to 520ksps.

# Usage

```
plutosdr -f 434000000 -s 580000 -g 0.0 -b 4096 | gzip > file.raw.gz
```

# Installation

Make sure libiio installed. See [official documentation](https://github.com/analogdevicesinc/libiio) on how to do it. Make sure the latest version installed (at least 0.21).

```
git clone https://github.com/dernasherbrezon/plutosdr.git
cd plutosdr
mkdir build
cd build
cmake ..
make
sudo make install
```