
#include <iostream>
#include <thread>

#include <QString>
#include <QFileDialog>

#include "BagViewer.h"
#include "ui_BagViewer.h"

#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>
#include <cv_bridge/cv_bridge.h>

#include <opencv2/highgui.hpp>

#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


using namespace std;


using ptime = boost::posix_time::ptime;
using tduration = boost::posix_time::time_duration;


inline ptime getCurrentTime ()
{ return boost::posix_time::microsec_clock::local_time(); }


BagViewer::BagViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::BagViewer)
{
    ui->setupUi(this);
}


BagViewer::~BagViewer()
{
    delete ui;
}


void
BagViewer::setBagFile(const std::string &bagfilename)
{
	bagFdPtr = std::shared_ptr<rosbag::Bag>(new rosbag::Bag(bagfilename, rosbag::BagMode::Read));

	ui->topicSelector->clear();

	auto vTopicList = RandomAccessBag::getTopicList(*bagFdPtr);
	for (auto vi: vTopicList) {

		if (vi.second=="sensor_msgs/Image" or vi.second=="sensor_msgs/CompressedImage") {
			RandomAccessBag::Ptr smImg (new RandomAccessBag(*bagFdPtr, vi.first));
			imageBagList.push_back(smImg);
			ui->topicSelector->addItem(QString(vi.first.c_str()));
		}
	}

	setTopic(0);
}


void
BagViewer::setTopic(int n)
{
	if (n>=imageBagList.size())
		return;

	currentActiveTopic = imageBagList.at(n);
	ui->playProgress->setRange(0, currentActiveTopic->size()-1);
	ui->playProgress->setValue(0);

	updateImage(0);
}


void
BagViewer::on_playButton_clicked(bool checked)
{
	static bool playStarted = false;
	static std::thread *playerThread = NULL;

	std::function<void()> playThreadFn =
	[&]()
	{
		const int startPos = ui->playProgress->sliderPosition();
		disableControlsOnPlaying(true);
		for (int p=startPos; p<=ui->playProgress->maximum(); p++) {

			ptime t1x = getCurrentTime();
			ui->playProgress->setSliderPosition(p);
			updateImage(p);
			if (playStarted == false)
				break;

			if(p < ui->playProgress->maximum()) {
				ros::Time
					t1 = currentActiveTopic->timeAt(p),
					t2 =  currentActiveTopic->timeAt(p+1);
				ptime t2x = getCurrentTime();
				tduration tdx = t2x - t1x;	// processing overhead
				tduration td = (t2-t1).toBoost() - tdx;
				std::this_thread::sleep_for(std::chrono::milliseconds(td.total_milliseconds()));
			}
		}
		disableControlsOnPlaying(false);
	};

	if (checked==true) {
		playStarted = true;
		playerThread = new std::thread(playThreadFn);
	}

	else {
		playStarted = false;
		playerThread->join();
		delete(playerThread);
	}

	return;
}


void
BagViewer::on_playProgress_sliderMoved(int i)
{
	updateImage(i);
}


void
BagViewer::on_topicSelector_currentIndexChanged(int i)
{
	setTopic(i);
}


void
BagViewer::on_saveButton_clicked(bool checked)
{
	QString fname = QFileDialog::getSaveFileName(this, tr("Save Image"));
	if (fname.length()==0)
		return;

	cv::Mat rgb;
	cv::cvtColor(currentImage, rgb, CV_RGB2BGR);

	cv::imwrite(fname.toStdString(), rgb);
}


void
BagViewer::updateImage(int n)
{
	if (n>=currentActiveTopic->size())
		return;

	if (currentActiveTopic->messageType()=="sensor_msgs/Image") {
		sensor_msgs::Image::ConstPtr imageMsg = currentActiveTopic->at<sensor_msgs::Image>(n);
		currentImage = cv_bridge::toCvCopy(imageMsg, sensor_msgs::image_encodings::RGB8)->image;

	}

	else if (currentActiveTopic->messageType()=="sensor_msgs/CompressedImage") {
		sensor_msgs::CompressedImage::ConstPtr imageMsg = currentActiveTopic->at<sensor_msgs::CompressedImage>(n);
		currentImage = cv_bridge::toCvCopy(imageMsg, sensor_msgs::image_encodings::RGB8)->image;
	}

	QImage curImage (currentImage.data, currentImage.cols, currentImage.rows, currentImage.step[0], QImage::Format_RGB888);
	ui->imageFrame->setImage(curImage);
}


void
BagViewer::disableControlsOnPlaying(bool state)
{
	ui->topicSelector->setDisabled(state);
	ui->saveButton->setDisabled(state);
	ui->playProgress->setDisabled(state);
}
