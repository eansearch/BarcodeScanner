//////////////////////////////////////////////////////////////////
//
// BarcodeScanner
// Scan and lookup barcodes
//
// Copyright (c) 2021, Jan Willamowius <jan@willamowius.de>
//
// This work is published under the GNU Public License
//
//////////////////////////////////////////////////////////////////

#include "BarcodeScanner.h"
#include <QtWidgets/QApplication>
#include <QDir>
#include <gst/gst.h>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	// Initialize GStreamer
#ifdef Q_OS_WIN
	QString pathString = QDir::currentPath();
	QByteArray pathByteArray;
	pathByteArray.append(pathString);
	qputenv("GSTREAMER_1_0_ROOT_X86", pathByteArray);
	qputenv("GST_PLUGIN_PATH", pathByteArray);
#endif
	gst_init(&argc, &argv);

	BarcodeScanner w;
	w.show();
	int code = a.exec();
	//gst_deinit(); // only needed to help Valgrind
	return code;
}
