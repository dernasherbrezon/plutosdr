# About

Set of programs to quickly receive and transmit data using pluto-sdr.

# Features

 * RX - receive cs16 into the specified file out stdout
 * TX - transmit cu8 or cs16 files
 * Support sampling rates from 61.44MSPS to 65.105KSPS. This is archived using decimating/interpolating filters and extra 8x decimation/interpolation in HDL

# Usage


```bash
pluto_tx -f 868000000 -s 240000 -b 120000 -i file.cu8
```

```bash
pluto_rx -f 868000000 -g 40.0 -s 240000 -b 120000 | gzip > file.cs16.gz
```

# Installation

Make sure [libiio](https://github.com/analogdevicesinc/libiio) installed.

```
mkdir build
cd build
cmake ..
make
```