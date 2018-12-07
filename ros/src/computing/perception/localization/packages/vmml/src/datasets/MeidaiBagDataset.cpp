/*
 * MeidaiBag.cpp
 *
 *  Created on: Aug 10, 2018
 *      Author: sujiwo
 */

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/filesystem.hpp>

#include <exception>
#include <algorithm>
#include <fstream>
#include <string>

#include <ros/ros.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

#include "datasets/MeidaiBagDataset.h"


using namespace std;
using namespace Eigen;

namespace bfs = boost::filesystem;

string MeidaiBagDataset::dSetName = "Nagoya University";

#define _DashboardMask "conf/meidai_mask.png"
#define _ExposureAdjustmentMask "conf/meidai_exposure_adjust.png"


const string
	meidaiBagImageTopic = "/camera1/image_raw",
	meidaiBagGnssTopic  = "/nmea_sentence",
	meidaiBagVelodyne   = "/velodyne_packets";


const TTransform defaultLidarToCameraTransform =
	TTransform::from_XYZ_RPY(
		Vector3d(0.9, 0.3, -0.6),
		-1.520777, -0.015, -1.5488);


class cache_error : public runtime_error
{};


MeidaiBagDataset::MeidaiBagDataset(

	const string &path,
/*
	double startTimeOffsetSecond,
	double mappingDurationSecond,
	const std::string &calibrationPath,
*/
	bool loadPositions) :

		bagPath(path),
		bagfd(new rosbag::Bag(path))
{
	prepareBag();

	if (loadPositions==true)
		loadCache();

	// try to load mask image
	bfs::path myPath = getMyPath();
	auto mask1Path = myPath / _DashboardMask;
	auto mask2Path = myPath / _ExposureAdjustmentMask;
	dashBoardMask = cv::imread(mask1Path.string(), cv::IMREAD_GRAYSCALE);
/*
	if (dashBoardMask.empty())
		throw runtime_error("Unable to load Meidai car image");
*/
	exposureMask = cv::imread(mask2Path.string(), cv::IMREAD_GRAYSCALE);
/*
	if (exposureMask.empty())
		throw runtime_error("Unable to load Meidai exposure mask");
*/

	mPreprocessor.setMode(ImagePreprocessor::ProcessMode::AGC);
	mPreprocessor.setMask(exposureMask);
}


MeidaiBagDataset::Ptr
MeidaiBagDataset::load (
		const string &path,
/*
		double startTimeOffsetSecond,
		double mappingDurationSecond,
		const std::string &calibrationPath,
*/
		bool loadPositions
)
{
	MeidaiBagDataset::Ptr nuDatasetMem(new MeidaiBagDataset(
		path,
//		startTimeOffsetSecond,
//		mappingDurationSecond,
//		calibrationPath,
		loadPositions));
	return nuDatasetMem;
}


MeidaiBagDataset::MeidaiBagDataset
(const MeidaiBagDataset &cp):
	bagPath(cp.bagPath),
	bagfd(cp.bagfd),
	cameraParams(cp.cameraParams),
	zoomRatio(cp.zoomRatio),
	dashBoardMask(cp.dashBoardMask),
	exposureMask(cp.exposureMask),
	mPreprocessor(cp.mPreprocessor)
{}


/*
MeidaiBagDataset::Ptr
MeidaiBagDataset::subset(const ros::Time &startTime, const ros::Duration &lengthInSecond) const
{
	MeidaiBagDataset *rsubset = new MeidaiBagDataset( *this );
	rsubset->isSubset_ = true;

	ros::Time tlast = startTime + lengthInSecond;
	rsubset->subsetBeginTime = startTime;
	rsubset->subsetEndTime = tlast;
	rsubset->prepareBag(startTime, tlast);

	// Load positions, if they are complete in this time range
	if (cameraTrack.empty())
		goto finish;

	if (cameraTrack.front().timestamp > startTime or cameraTrack.back().timestamp < tlast)
		goto finish;

	rsubset->cameraTrack = cameraTrack.subset(startTime, tlast);

finish:
	return MeidaiBagDataset::Ptr(rsubset);
}


MeidaiBagDataset::Ptr
MeidaiBagDataset::subset(const double startTimeOffsetSecond, const double endOffsetFromBeginning) const
{
	assert (endOffsetFromBeginning >= startTimeOffsetSecond);

	ros::Duration
		d0(startTimeOffsetSecond),
		d1(endOffsetFromBeginning-startTimeOffsetSecond);
	ros::Time t1 = cameraRawBag->timeAt(0) + d0;

	return subset(t1, d1);
}
*/


void
MeidaiBagDataset::prepareBag (const ros::Time &beginTime, const ros::Time &stopTime)
{
/*
	cameraRawBag = RandomAccessBag::Ptr(new RandomAccessBag(*bagfd, meidaiBagImageTopic, beginTime, stopTime));
	gnssBag = RandomAccessBag::Ptr(new RandomAccessBag(*bagfd, meidaiBagGnssTopic, beginTime, stopTime));
	velodyneBag = RandomAccessBag::Ptr(new RandomAccessBag(*bagfd, meidaiBagVelodyne, beginTime, stopTime));
*/
	cameraRawBag = RandomAccessBag::Ptr(new RandomAccessBag(*bagfd, meidaiBagImageTopic));
	gnssBag = RandomAccessBag::Ptr(new RandomAccessBag(*bagfd, meidaiBagGnssTopic));
	velodyneBag = RandomAccessBag::Ptr(new RandomAccessBag(*bagfd, meidaiBagVelodyne));
}


void
MeidaiBagDataset::setTimeConstraint(const ptime &start, const ptime &stop)
{
//	cameraRawBag->
}


void
MeidaiBagDataset::loadPosition()
{
	loadCache();
}


MeidaiBagDataset::~MeidaiBagDataset()
{
}


size_t
MeidaiBagDataset::size() const
{
	return cameraRawBag->size();
}


size_t
MeidaiBagDataset::sizeAll() const
{
	RandomAccessBag bg(*bagfd, meidaiBagImageTopic);
	return bg.size();
}


CameraPinholeParams
MeidaiBagDataset::getCameraParameter()
const
{
	return cameraParams * zoomRatio;
}


void
MeidaiBagDataset::addCameraParameter(const CameraPinholeParams &c)
{ cameraParams = c; }


cv::Mat
MeidaiBagDataset::getMask()
{
	cv::Mat imgrs;
	if (zoomRatio==1.0)
		return dashBoardMask.clone();
	else {
		cv::resize(dashBoardMask, imgrs, cv::Size(), zoomRatio, zoomRatio, cv::INTER_CUBIC);
		return imgrs;
	}
}


MeidaiDataItem&
MeidaiBagDataset::at(dataItemId i) const
{
	// XXX: Stub
	throw runtime_error("Not implemented");
}


GenericDataItem::ConstPtr
MeidaiBagDataset::get(dataItemId i)
const
{
	MeidaiDataItem::ConstPtr dp(new MeidaiDataItem(*this, i));
	return dp;
}


float
MeidaiBagDataset::getZoomRatio () const
{ return zoomRatio; }


void
MeidaiBagDataset::loadCache()
{
	bfs::path bagCachePath = bagPath;
	bagCachePath += ".cache";
	bool isCacheValid = false;

	// Check if cache is valid
	if (bfs::exists(bagCachePath) and bfs::is_regular_file(bagCachePath)) {
		auto lastWriteTime = bfs::last_write_time(bagCachePath),
			lastBagModifyTime = bfs::last_write_time(bagPath);
		if (lastWriteTime >= lastBagModifyTime)
			isCacheValid = true;
	}

	if (isCacheValid) {
		doLoadCache(bagCachePath.string());
	}
}


void
MeidaiBagDataset::setLidarParameters (
	const std::string &pvelodyneCalibrationFile,
	const std::string &pmeidaiPCDMapFile,
	const TTransform &plidarToCameraTransform)
{
	lidarToCameraTransform = plidarToCameraTransform;
	pcdMapFilePath = pmeidaiPCDMapFile;
	velodyneCalibrationFilePath = pvelodyneCalibrationFile;
}


/*
void
MeidaiBagDataset::forceCreateCache (bool resetSubset, bool useNdt)
{
	bfs::path bagCachePath = bagPath;
	bagCachePath += ".cache";

	if (resetSubset==true) {
		isSubset_ = false;
		subsetBeginTime = ros::TIME_MIN;
		subsetEndTime = ros::TIME_MIN;
	}

	gnssTrack.clear();
	ndtTrack.clear();
	cameraTrack.clear();
	createTrajectories(useNdt);
	writeCache(bagCachePath.string());
}
*/


void
MeidaiBagDataset::forceCreateCache (bool useLidar, const double startOffset, const double stopOffset)
{
	ptime
		t1 = cameraRawBag->timeFromStart(startOffset).toBoost(),
		t2 = cameraRawBag->timeFromStart(stopOffset).toBoost();
	return forceCreateCache(useLidar, t1, t2);
}


void
MeidaiBagDataset::forceCreateCache (bool useLidar, const ptime &t1, const ptime &t2)
{
	bfs::path bagCachePath = bagPath;
	bagCachePath += ".cache";

	gnssTrack.clear();
	ndtTrack.clear();
	cameraTrack.clear();

	ros::Time
		tr1 = (t1==MIN_TIME ? ros::TIME_MIN : ros::Time::fromBoost(t1)),
		tr2 = (t2==MAX_TIME ? ros::TIME_MAX : ros::Time::fromBoost(t2)),

	createTrajectories(tr1, tr2, useLidar);
	writeCache(bagCachePath.string());
}


void
MeidaiBagDataset::doLoadCache(const string &path)
{
	fstream cacheFd;
	cacheFd.open(path.c_str(), fstream::in);
	if (!cacheFd.is_open())
		throw runtime_error(string("Unable to open cache file: ") + path);

	boost::archive::binary_iarchive cacheIArc (cacheFd);

/*
	cacheIArc >> isSubset_;
	ptime tx;
	cacheIArc >> tx;
	subsetBeginTime = ros::Time::fromBoost(tx);
	cacheIArc >> tx;
	subsetEndTime = ros::Time::fromBoost(tx);

	if (isSubset_) {
		prepareBag(subsetBeginTime, subsetEndTime);
	}
*/

	cacheIArc >> gnssTrack;
	cacheIArc >> ndtTrack;
	cacheIArc >> cameraTrack;

	cacheFd.close();
}


void
MeidaiBagDataset::createTrajectories(ros::Time startTime, ros::Time stopTime, bool useLidar)
{
	Trajectory *trajectorySrc;
	bool doCompensateTime = false;

	if (startTime==ros::TIME_MIN and stopTime==ros::TIME_MAX) {
		ros::Duration td(0.25);
		doCompensateTime = true;
		startTime -= td;
		stopTime += td;
	}

	/*
	 * Create full-bag trajectory for GNSS. It will be used as fallback
	 * when LIDAR localization fails
	 */
	cout << "Creating GNSS Trajectory\n";
	createTrajectoryFromGnssBag(*gnssBag, gnssTrack);

	/*
	 * As of December 2018, NDT localization still could fail undetected (unless it drifts out-of-the-map)
	 */
	if (useLidar==true) {
		cout << "Creating NDT Trajectory\n";
		auto lidarBag = getLidarScanBag();

		if (doCompensateTime)
			lidarBag->setTimeConstraint(startTime, stopTime);

		createTrajectoryFromNDT(*lidarBag, ndtTrack, gnssTrack, pcdMapFilePath);
		trajectorySrc = &ndtTrack;
	}

	else
		trajectorySrc = &gnssTrack;

	cout << "Creating Camera Trajectory\n";
	// XXX: It is possible that camera recording may have started earlier than lidar's

	if (doCompensateTime)
		cameraRawBag->setTimeConstraint(startTime, stopTime);

	for (int i=0; i<cameraRawBag->size(); i++) {
		auto tm = cameraRawBag->timeAt(i).toBoost();

		// in both cases; extrapolate
		PoseStamped poseX;
		if (tm < trajectorySrc->at(0).timestamp or tm>=trajectorySrc->back().timestamp) {
			poseX = trajectorySrc->extrapolate(tm);
		}
		else
			poseX = trajectorySrc->interpolate(tm);
		// XXX: Check this value
		PoseStamped poseX1 = poseX * lidarToCameraTransform;
		cameraTrack.push_back(poseX1);

		cout << i+1 << " / " << cameraRawBag->size() << "  \r";
	}
}


void MeidaiBagDataset::writeCache(const string &path)
{
	fstream cacheFd;
	cacheFd.open(path.c_str(), fstream::out);
	if (!cacheFd.is_open())
		throw runtime_error(string("Unable to open cache file: ") + path);

	boost::archive::binary_oarchive cacheOArc (cacheFd);

/*
	cacheOArc << isSubset_;
	ptime tx = subsetBeginTime.toBoost();
	cacheOArc << tx;
	tx = subsetEndTime.toBoost();
	cacheOArc << tx;
*/

	cacheOArc << gnssTrack;
	cacheOArc << ndtTrack;
	cacheOArc << cameraTrack;

	cacheFd.close();
}


GenericDataItem::ConstPtr
MeidaiBagDataset::atDurationSecond (const double second)
const
{
	uint32_t pos = cameraRawBag->getPositionAtDurationSecond(second);
	return get(pos);
}


dataItemId
MeidaiBagDataset::getLowerBound (const ptime &t) const
{
	auto rt = ros::Time::fromBoost(t);
	return (dataItemId)cameraRawBag->getPositionAtTime(rt);
}



LidarScanBag::Ptr
MeidaiBagDataset::getLidarScanBag ()
{
	if (velodyneCalibrationFilePath.empty())
		throw runtime_error("Velodyne calibration file not set");

	ros::Time
		tstart = ros::TIME_MIN,
		tstop = ros::TIME_MAX;
/*
	if (isSubset()) {
		tstart = subsetBeginTime;
		tstart = subsetEndTime;
	}
*/

	return LidarScanBag::Ptr (new LidarScanBag(*bagfd, meidaiBagVelodyne, velodyneCalibrationFilePath, tstart, tstop));
}


cv::Mat
MeidaiBagDataset::computeImagePreprocessorMask()
const
{
	// XXX: Stub
}


Trajectory
MeidaiBagDataset::getCameraTrajectory(const ptime timeStart, const ptime timeStop) const
{

}

/*
void
Trajectory::push_back(const PoseTimestamp &pt)
{
	if (size()>0)
		assert(pt.timestamp >= back().timestamp);
	return Parent::push_back(pt);
}


uint32_t
Trajectory::find_lower_bound(const ros::Time &t) const
{
	if ( t < front().timestamp or t > back().timestamp )
		throw out_of_range("Time out of range");

	auto it = std::lower_bound(begin(), end(), t,
		[](const PoseTimestamp &el, const ros::Time& tv)
			-> bool {return el.timestamp < tv;}
	);
	uint32_t x = it-begin();
	return x;
}


uint32_t
Trajectory::find_lower_bound(const ptime &t) const
{
	ros::Time tx = ros::Time::fromBoost(t);
	return find_lower_bound(tx);
}


PoseTimestamp
Trajectory::at(const ros::Time& t) const
{
	return Parent::at(find_lower_bound(t));
}


PoseTimestamp
Trajectory::at(const ptime &t) const
{

	return Parent::at(find_lower_bound(t));
}


PoseTimestamp
Trajectory::interpolate (const ros::Time& t) const
{
	assert (front().timestamp<=t and t < back().timestamp);

	uint32_t i0 = find_lower_bound(t)-1;
	uint32_t i1 = i0+1;
	return PoseTimestamp::interpolate(Parent::at(i0), Parent::at(i1), t);
}


PoseTimestamp
Trajectory::extrapolate (const ros::Time& t) const
{
	assert (t < front().timestamp or t > back().timestamp);

	double r;
	Vector3d xpos;
	Quaterniond xori;

	if (t > back().timestamp) {
		size_t sz = size();
		const PoseTimestamp
			&lastPose = back(),
			&lastPose_1 = Parent::at(sz-2);

		r = (t.toSec() - lastPose_1.timestamp.toSec()) /
			(lastPose.timestamp.toSec() - lastPose_1.timestamp.toSec());

		xpos = lastPose_1.position() + r*(lastPose.position() - lastPose_1.position());
		xori = PoseTimestamp::extrapolate(lastPose_1, lastPose, t);
	}

	else if (t < front().timestamp) {
		const PoseTimestamp
			&firstPose = front(),
			&firstPose_1 = Parent::at(1);

		r = (t.toSec() - firstPose_1.timestamp.toSec()) /
			(firstPose.timestamp.toSec() - firstPose_1.timestamp.toSec());
		xpos = firstPose_1.position() + r*(firstPose.position() - firstPose_1.position());
		xori = PoseTimestamp::extrapolate(firstPose, firstPose_1, t);
	}

	return PoseTimestamp(xpos, xori, t);
}


Trajectory
Trajectory::subset(const ros::Time &start, const ros::Time &stop) const
{
	assert(start>=front().timestamp and stop<=back().timestamp);

	Trajectory ssub;
	for (auto it=begin(); it!=end(); ++it) {
		auto &p = *it;
		if (start<=p.timestamp and p.timestamp<=stop)
			ssub.push_back(p);
	}

	return ssub;
}
*/


double normalizeAngle(const double &r)
{
	if (r > 2*M_PI) {

	}

	else if (r<0) {

	}

	else return r;
}


/*
Quaterniond
PoseTimestamp::extrapolate(const PoseTimestamp &p1, const PoseTimestamp &p2, const decltype(PoseTimestamp::timestamp) &txinp)
{
	assert (p1.timestamp < p2.timestamp);
	if (p1.timestamp <= txinp and txinp <= p2.timestamp)
		throw runtime_error("Requested timestamp are within both data points; use interpolation instead");

	Vector3d euler1, euler2, eulerx;
	euler1 = quaternionToRPY(p1.orientation());
	euler2 = quaternionToRPY(p2.orientation());
	double t1, t2, tx, r;
	t1 = p1.timestamp.toSec();
	t2 = p2.timestamp.toSec();
	tx = txinp.toSec();

	if (txinp<=p1.timestamp) {
		r = (t1 - tx) / (t2 - t1);
		eulerx.x() = euler1.x() - r*(euler2.x() - euler1.x());
		eulerx.y() = euler1.y() - r*(euler2.y() - euler1.y());
		eulerx.z() = euler1.z() - r*(euler2.z() - euler1.z());
	}

	else {
		r = (tx - t1) / (t2 - t1);
		eulerx.x() = euler1.x() + r*(euler2.x() - euler1.x());
		eulerx.y() = euler1.y() + r*(euler2.y() - euler1.y());
		eulerx.z() = euler1.z() + r*(euler2.z() - euler1.z());
	}

//	eulerx.x() = normalizeAngle(eulerx.x());
//	eulerx.y() = normalizeAngle(eulerx.y());
//	eulerx.z() = normalizeAngle(eulerx.z());

	return fromRPY(eulerx.x(), eulerx.y(), eulerx.z());
}
*/


void
MeidaiDataItem::init()
{
	bImageMsg = parent.cameraRawBag->at<sensor_msgs::Image>(pId);
}


Pose
MeidaiDataItem::getPose()
const
{
	auto &p = parent.cameraTrack[pId];
	return p;
}


Vector3d
MeidaiDataItem::getPosition() const
{
	auto &p = parent.cameraTrack[pId];
	return p.position();
}


Quaterniond
MeidaiDataItem::getOrientation() const
{
	auto &p = parent.cameraTrack[pId];
	return p.orientation();
}


cv::Mat
MeidaiDataItem::getImage() const
{
	auto imgPtr = cv_bridge::toCvCopy(bImageMsg, sensor_msgs::image_encodings::BGR8);

	if (parent.isPreprocessed) {
		parent.mPreprocessor.preprocess(imgPtr->image);
	}

	cv::Mat imgrs;
	if (parent.zoomRatio==1.0)
		return imgPtr->image;
	else {
		cv::resize(imgPtr->image, imgrs, cv::Size(), parent.zoomRatio, parent.zoomRatio, cv::INTER_CUBIC);
	}

	return imgrs;
}


ptime
MeidaiDataItem::getTimestamp() const
{
	return bImageMsg->header.stamp.toBoost();
}


bool
MeidaiBagDataset::isCameraTrajectoryComplete() const
{
	if (cameraTrack.empty())
		return false;

	if (cameraTrack[0].timestamp > cameraRawBag->startTime().toBoost() or
		cameraTrack.back().timestamp < cameraRawBag->stopTime().toBoost())
	return false;

	return true;
}


/*
PoseTimestamp
PoseTimestamp::operator* (const Pose &transform)
{
//	Pose me = Pose::from_Pos_Quat(this->position(), this->orientation());
	Pose P = static_cast<Pose&>(*this) * transform;
	return PoseTimestamp(P, this->timestamp);
}


PoseTimestamp
PoseTimestamp::interpolate(
	const PoseTimestamp &p1,
	const PoseTimestamp &p2,
	const ros::Time &t)
{
	assert (p1.timestamp<=t and t<=p2.timestamp);
	double r = (t - p1.timestamp).toSec() / (p2.timestamp - p1.timestamp).toSec();
	return PoseTimestamp (Pose::interpolate(p1, p2, r), t);
}
*/

