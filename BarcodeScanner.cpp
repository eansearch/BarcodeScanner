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

#include "BarcodeScanner.h"

#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QtWidgets>
#include <QMessageBox>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSettings>

#include <gst/video/videooverlay.h>

#ifdef Q_OS_WINDOWS
#define strcasecmp stricmp
#endif

BarcodeScanner::BarcodeScanner(QWidget *parent)
	: QDialog(parent)
{
    settings = new QSettings("ean-search.org", "BarcodeScanner");
	// Qt dialog init
	camera = new QComboBox();
	QFont cameraFont = camera->font();
	cameraFont.setPointSize(FONT_SIZE);
	camera->setFont(cameraFont);

	// camera device detection
	GstDeviceMonitor* monitor = gst_device_monitor_new();
	gst_device_monitor_add_filter(monitor, "Video/Source", NULL);
	if (gst_device_monitor_start(monitor)) {
		GList* devices = gst_device_monitor_get_devices(monitor);
		for (GList* devIter = g_list_first(devices); devIter != NULL; devIter = g_list_next(devIter)) {
			GstDevice* device = (GstDevice*)devIter->data;
			if (device == NULL)
				continue;
			gchar* name = gst_device_get_display_name(device);
			const gchar* device_handle = name; // name on Windows, device path on Linux
			GstStructure* props = gst_device_get_properties(device);
			if (props) {
				const gchar* api = gst_structure_get_string(props, "device.api");
				if (api && strcmp("v4l2", api) == 0) {
					device_handle = gst_structure_get_string(props, "device.path");
				}
			}
			QString data;
			QTextStream(&data) << device_handle << "|" << "";
			camera->addItem(name, data);

			g_free(name);
			if (props)
				gst_structure_free(props);
		}
		g_list_free(devices);
		gst_device_monitor_stop(monitor);
	}
	gst_object_unref(monitor);

    eanSearchToken = new QLineEdit();
    QFont tokenFont = eanSearchToken->font();
    tokenFont.setPointSize(FONT_SIZE);
    eanSearchToken->setFont(tokenFont);
    eanSearchToken->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    eanSearchToken->setMinimumWidth(500);
    eanSearchToken->setText(settings->value("token", "").toString());

	errorMsg = new QLabel("");
	errorMsg->setStyleSheet("QLabel { color : red; }");

	startButton = new QPushButton(tr("  Start scanning  "));
	QFont largetFont = startButton->font();
	largetFont.setPointSize(FONT_SIZE);
	startButton->setFont(largetFont);
	stopButton = new QPushButton(tr("  Stop scanning  "));
	stopButton->setFont(largetFont);
	stopButton->setEnabled(false);
	aboutButton = new QPushButton(tr("  About  "));
	aboutButton->setFont(largetFont);
	quitButton = new QPushButton(tr("Quit"));
	quitButton->setFont(largetFont);

    productInfo = new QLabel(" \n ");
    productInfo->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    productInfo->setMinimumWidth(500);
    productInfo->setMinimumHeight(5 * FONT_SIZE);
    productInfo->setStyleSheet("QLabel { color : blue; }");
    productInfo->setTextFormat(Qt::RichText);
    productInfo->setTextInteractionFlags(Qt::TextBrowserInteraction);
    productInfo->setOpenExternalLinks(true);

    videoArea = new QLabel("");
    videoArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    videoArea->setMinimumWidth(640);
    videoArea->setMinimumHeight(480);
    videoArea->setAttribute(Qt::WA_NativeWindow); // deeded for GStreamer targets

	connect(startButton, SIGNAL(clicked()), this, SLOT(startClicked()));
	connect(stopButton, SIGNAL(clicked()), this, SLOT(stopClicked()));
	connect(aboutButton, SIGNAL(clicked()), this, SLOT(aboutClicked()));
	connect(quitButton, SIGNAL(clicked()), this, SLOT(quitClicked()));

	QVBoxLayout* mainLayout = new QVBoxLayout();
	QGroupBox* camSettingsBox = new QGroupBox();
	QHBoxLayout* camSettingsBoxLayout = new QHBoxLayout();

	camSettingsBoxLayout->addWidget(camera);
	camSettingsBox->setLayout(camSettingsBoxLayout);
	camSettingsBox->setTitle(tr("Camera Settings"));
	mainLayout->addWidget(camSettingsBox);

	QGroupBox* tokenBox = new QGroupBox();
	QHBoxLayout* tokenBoxLayout = new QHBoxLayout();

	tokenBoxLayout->addWidget(eanSearchToken);
	tokenBox->setLayout(tokenBoxLayout);
	tokenBox->setTitle(tr("ean-search.org API Token (optional)"));
	mainLayout->addWidget(tokenBox);

	mainLayout->addWidget(errorMsg);
	mainLayout->addSpacing(FONT_SIZE / 2);

    QGridLayout * buttonLayout = new QGridLayout();
	buttonLayout->addWidget(startButton,0, 0);
	buttonLayout->addWidget(stopButton, 0, 1);
	buttonLayout->addWidget(aboutButton, 1, 0);
	buttonLayout->addWidget(quitButton, 1, 1);
    mainLayout->addLayout(buttonLayout);
	mainLayout->addSpacing(FONT_SIZE);

	QGroupBox* productBox = new QGroupBox();
	QHBoxLayout* productBoxLayout = new QHBoxLayout();
	productBoxLayout->addWidget(productInfo);
	productBox->setLayout(productBoxLayout);
	productBox->setTitle(tr("Product Information"));
	mainLayout->addWidget(productBox);
	mainLayout->addSpacing(FONT_SIZE);

	mainLayout->addWidget(videoArea);

	mainLayout->addStretch();

	setLayout(mainLayout);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // remove "?" button
	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint); // add minimize button
	setWindowTitle(tr("BarcodeScanner") + " " + VERSION);
	setFixedWidth(sizeHint().width());
	setFixedHeight(sizeHint().height());

	// can't start without any camera
	if (!camera->count()) {
		camera->setEnabled(false);
		startButton->setEnabled(false);
	}
}

BarcodeScanner::~BarcodeScanner()
{
    settings->setValue("token", eanSearchToken->text());
    delete settings;
}

void BarcodeScanner::dispayErrorAndExit(const QString & msg)
{
	QMessageBox::critical(this, "Terminating", msg);
	exit(1);
}

void BarcodeScanner::startClicked()
{
	startButton->setEnabled(false);
	stopButton->setEnabled(true);
    stopButton->setFocus(Qt::OtherFocusReason);
	camera->setEnabled(false);

	QStringList camera_data = camera->currentData().toString().split("|");
	QString camera_handle = camera_data[0];

    productInfo->setText("");

	// Build the pipeline
#ifdef Q_OS_UNIX
	QString videoSrc = "v4l2src do-timestamp=true name=videosrc";
    QString videoSink = "xvimagesink name=imagesink";
#else
	QString videoSrc = "ksvideosrc do-stats=true name=videosrc";
    QString videoSink = "autovideosink"; // TODO: can't be embedded into dialog, detected by missing name=
#endif
	//QString pipeline_str = videoSrc + QString(" ! videoconvert ! video/x-raw,width=640,height=480,framerate=10/1 ! zbar name=zbar ! videoconvert ! autovideosink");
	QString pipeline_str = videoSrc
                        + QString(" ! videoconvert ! video/x-raw,width=640,height=480,framerate=10/1 ! zbar name=zbar ! videoconvert ! ")
                        + videoSink;
	
	//QMessageBox::information(this, "Debug: Pipeline", pipeline_str); // debug

	GError *error = NULL;
	pipeline = gst_parse_launch((const char *)pipeline_str.toLatin1(), &error);
	if (!pipeline || error) {
		dispayErrorAndExit(QString("Pipeline error: ") + error->message + "\nIs GStreamer installed corectly ?");
	}
	bus = gst_element_get_bus(pipeline);
	gst_event_timer = new QTimer(this);
	connect(gst_event_timer, SIGNAL(timeout()), this, SLOT(checkGstBus()));

	// select camera
	GstElement* videosrc = gst_bin_get_by_name(GST_BIN(pipeline), "videosrc");
	if (!videosrc) {
		dispayErrorAndExit("Error: Can't get videosrc!");
	}

#ifdef Q_OS_UNIX
	g_object_set(videosrc, "device", camera_handle.toStdString().c_str(), NULL); // v4l2
#endif
#ifdef Q_OS_WINDOWS
	g_object_set(videosrc, "device-name", camera_handle.toStdString().c_str(), NULL); // ksvideo
#endif
	gst_object_unref(videosrc);

	GstElement* imagesink = gst_bin_get_by_name(GST_BIN(pipeline), "imagesink");
	if (!imagesink) {
		videoArea->hide();
        setFixedHeight(sizeHint().height());
	} else {
	    gst_element_set_state(imagesink, GST_STATE_READY);
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(imagesink) , videoArea->winId());
	    gst_object_unref(imagesink);
    }

	// Start the pipeline
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	gst_event_timer->start(POLL_TIMER);
}

void BarcodeScanner::stopClicked()
{
	startButton->setEnabled(true);
    startButton->setFocus(Qt::OtherFocusReason);
	stopButton->setEnabled(false);
	camera->setEnabled(true);
	errorMsg->setText("");
    last_symbol = "";

	// Stop the pipeline
	gst_event_timer->stop();
	delete gst_event_timer;
	gst_event_timer = NULL;
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(bus);
	bus = NULL;
	gst_object_unref(pipeline);
	pipeline = NULL;
}

void BarcodeScanner::aboutClicked()
{
	//QMessageBox::about(this, "ean-search.org Barcode Scanner", "&copy; 2022 Relaxed Comunications GmbH, https://www.ean-search.org/" );
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setWindowTitle("ean-search.org Barcode Scanner " + VERSION);
    msgBox->setText("Barcode Scanner " + VERSION + "<p>Get your API token from <a href='https://www.ean-search.org/ean-database-api.html'>ean-search.org</a> to automatically lookup the scanned barcodes.");
    msgBox->setInformativeText("&copy; 2022 Relaxed Comunications GmbH<br><a href='https://www.ean-search.org/'>www.ean-search.org</a>");
    msgBox->show();
}

void BarcodeScanner::quitClicked()
{
	BarcodeScanner::close();
}

void BarcodeScanner::checkGstBus()
{
	if (!gst_event_timer || !bus)
		return;

	gst_event_timer->stop();

	GstMessage* msg = gst_bus_pop_filtered(bus, GstMessageType(GST_MESSAGE_ELEMENT | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_CLOCK_LOST));
	if (msg) {
		if (msg && msg->type & GST_MESSAGE_ELEMENT) {
			const GstStructure* s = gst_message_get_structure(msg);
			const gchar* name = gst_structure_get_name(s);

			if (strcmp(name, "barcode") == 0) {
                const GValue* type_val = gst_structure_get_value(s, "type");
                const gchar * type = g_value_get_string(type_val);
                if (strcmp(type, "EAN-13") == 0 || strcmp(type, "UPC-A") == 0) {
                    const GValue* symbol_val = gst_structure_get_value(s, "symbol");
                    const gchar * symbol = g_value_get_string(symbol_val);
                    if (last_symbol != symbol) {
                        last_symbol = symbol;
                        EANLookup(symbol);
                    }
                } else if (strcmp(type, "EAN-8") == 0 || strcmp(type, "UPC-E") == 0) {
                    productInfo->setText(QString("Barcode ") + type + QString(" is not globally unique"));
                } else {
                    productInfo->setText(QString("") + type + QString(" is not an EAN/UPC product barcode"));
                }
            }
		}

		if (msg->type & GST_MESSAGE_EOS) {
			errorMsg->setText("Error: End of stream");
		}
		if (msg && msg->type & GST_MESSAGE_CLOCK_LOST) {
			/* Get a new clock */
			errorMsg->setText("Error: Lost clock, resetting");
			gst_element_set_state(pipeline, GST_STATE_PAUSED);
			gst_element_set_state(pipeline, GST_STATE_PLAYING);
		}

		if (msg->type & GST_MESSAGE_ERROR) {
			GError* err;
			gchar* debug;
			gst_message_parse_error(msg, &err, &debug);
			if (msg->src && msg->src->name) {
				errorMsg->setText(QString("Error in ") + msg->src->name + ": " +  err->message);
			}
			else {
				errorMsg->setText(QString("Error: ") + err->message);
			}
			g_error_free(err);
			g_free(debug);
		}

		gst_message_unref(msg);
	}

	gst_event_timer->start(POLL_TIMER);
}

void BarcodeScanner::EANLookup(const QString & ean) {
    QString token = eanSearchToken->text(); // get token from UI

    if (token.isEmpty()) {
        productInfo->setText("EAN " + ean + "<br>No API token for name lookup");
    } else {
        QNetworkRequest request = QNetworkRequest(QUrl("https://api.ean-search.org/api?token=" + token + "&format=json&op=barcode-lookup&ean=" + ean));
        QNetworkReply * reply = network_manager.get(request);
        request.setRawHeader("User-Agent", "BarcodeScanner (Qt)");
        // connect to signal when its done using lambda
        QObject::connect(reply, &QNetworkReply::finished, [=]() {
            QString ReplyText = reply->readAll();
            reply->deleteLater(); // clean up
            if (ReplyText.contains("error") && ReplyText.contains("Invalid token")) {
                productInfo->setText("EAN " + ean + "<br>Invalid <a href='https://www.ean-search.org/ean-database-api.html'>API</a> token");
                eanSearchToken->setText("");
                return;
            }
            QJsonDocument doc = QJsonDocument::fromJson(ReplyText.toUtf8());
            QJsonValue obj0 = doc.array()[0];
            QString name = obj0[QString("name")].toString();
            productInfo->setText("EAN " + ean + "<br><a href='https://www.ean-search.org/ean/" + ean + "'>" + name + "</a>");
        });
    }

}
