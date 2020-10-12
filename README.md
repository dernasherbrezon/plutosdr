# plutosdr-Cli

This program will configure fir filter for plutosdr and start reading the data. With decimating fir filter it is possible to set sample rate up to 520ksps.

# Usage

```
plutosdrCli -f 434000000 -s 580000 -g 0.0 -b 4096 | gzip > file.raw.gz
```

# Installation

Import launchpad repo. TBD