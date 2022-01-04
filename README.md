# Barcode Scanner for Linux

This barcode scanner uses the Webcam on your PC and is based on Qt and GStreamer. It may also works on Windows (untested).

It can also automatically look up the products for the scanned EAN and UPC barcodes online on https://www.ean-search.org/ .

## Compiling on Ubuntu Linux (tested with Ubuntu 21.10)

Open a terminal window:

git clone https://github.com/eansearch/BarcodeScanner

cd BarcodeScanner

sudo apt install qtbase5-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad

qmake

make

Then start it with

./BarcodeScanner &

