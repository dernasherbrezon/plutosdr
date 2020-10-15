# plutosdr

This program will configure fir filter for plutosdr and start reading the data. With decimating fir filter it is possible to set sample rate up to 520ksps.

# Usage

```
plutosdr -f 434000000 -s 580000 -g 0.0 -b 4096 | gzip > file.raw.gz
```

# Installation

```
sudo apt-get install dirmngr
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys A5A70917
sudo bash -c "echo 'deb [arch=armhf] http://s3.amazonaws.com/r2cloud r2cloud main' > /etc/apt/sources.list.d/r2cloud.list"
sudo apt-get update
sudo apt-get install plutosdr
```