//////////////////////////////////////////////////////////////////
//
// BarcodeScanner
// Scan and lookup barcodes
//
// Copyright (c) 2022, Relaxed Communications GmbH <info@relaxedcommunications.com>
//
// This work is published under the GNU Public License
//
//////////////////////////////////////////////////////////////////

#pragma once

#include <QtWidgets/QDialog>
#include <QNetworkAccessManager>
#include <QTimer>

#include <glib.h>
#include <gst/gst.h>

using namespace std;

class QLabel;
class QPushButton;
class QComboBox;
class QCheckBox;
class QLineEdit;
class QSettings;

const QString VERSION = "0.2";
const unsigned FONT_SIZE = 10;
const unsigned POLL_TIMER = 100; // 100 ms


class BarcodeScanner : public QDialog
{
	Q_OBJECT

public:
	BarcodeScanner(QWidget *parent = Q_NULLPTR);
	virtual ~BarcodeScanner();

protected:
	void dispayErrorAndExit(const QString & msg);
    void EANLookup(const QString & ean);

private slots:
	void startClicked();
	void stopClicked();
	void aboutClicked();
	void quitClicked();
	void checkGstBus();

protected:
    QSettings * settings;
    QString last_symbol;

    // QT UI
	QComboBox* camera;
    QLineEdit* eanSearchToken;

	QLabel* errorMsg;

	QPushButton* startButton;
	QPushButton* stopButton;
	QPushButton* aboutButton;
	QPushButton* quitButton;

    QLabel* productInfo;
    QNetworkAccessManager network_manager;

    QLabel* videoArea;

	// GStreamer data
	GstElement* pipeline;
	GstBus* bus;
	QTimer* gst_event_timer;
};
