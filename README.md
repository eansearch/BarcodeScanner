# Barcode Scanner for Linux

This barcode scanner uses the Webcam on your PC and is based on Qt and GStreamer. It may also works on Windows (untested).

It can also automatically look up the products for the scanned EAN and UPC barcodes online on https://www.ean-search.org/ .

## Compiling (tested with Ubuntu 21.10 and Mint 20.2)

Open a terminal window:

```bash
sudo apt install build-essential git qtbase5-dev libgstreamer1.0-dev \
		libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-base \
		gstreamer1.0-plugins-good gstreamer1.0-plugins-bad

git clone https://github.com/eansearch/BarcodeScanner

cd BarcodeScanner

qmake

make
```

Then start it with

```bash
./BarcodeScanner &
```

