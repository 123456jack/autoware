/*
 * MeidaiBag.h
 *
 *  Created on: Aug 10, 2018
 *      Author: sujiwo
 */

#ifndef _MEIDAIBAG_H_
#define _MEIDAIBAG_H_


#include <string>
#include <memory>
#include <vector>
#include <array>
#include <stdexcept>
#include <rosbag/bag.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>
#include <boost/filesystem.hpp>
#include <cv_bridge/cv_bridge.h>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

#include "utilities.h"
#include "ImagePreprocessor.h"
#include "Trajectory.h"
#include "datasets/GenericDataset.h"
#include "RandomAccessBag.h"
#include "datasets/LidarScanBag.h"


extern const TTransform defaultLidarToCameraTransform;


/*
struct PoseTimestamp : public Pose
{
	PoseTimestamp():
		Pose()
	{ timestamp = ros::Time(0); }

	PoseTimestamp(const Pose &p, const ros::Time &t=ros::Time(0))
	{
		m_matrix = p.matrix();
		timestamp = t;
	}

	PoseTimestamp (const Eigen::Vector3d &p, const Quaterniond &q, const ros::Time &t=ros::Time(0))
	{
		m_matrix = Pose::from_Pos_Quat(p, q).matrix();
		timestamp = t;
	}

	PoseTimestamp operator* (const Pose &t);

	double timeSecond () const
	{ return timestamp.toSec(); }

	static PoseTimestamp interpolate(
		const PoseTimestamp &p1,
		const PoseTimestamp &p2,
		const ros::Time &t);

	ros::Time timestamp;

	static
	Quaterniond
	extrapolate(const PoseTimestamp &p1, const PoseTimestamp &p2, const decltype(PoseTimestamp::timestamp) &tx);

	template<class Archive>
	inline void save(Archive &ar, const unsigned int v) const
	{
		// Eigen matrix
		ar << boost::serialization::base_object<Pose>(*this);

		// Ros timestamp
		ptime tx = timestamp.toBoost();
		ar << tx;
	}

	template<class Archive>
	inline void load(Archive &ar, const unsigned int v)
	{
		// Eigen matrix
		ar >> boost::serialization::base_object<Pose>(*this);

		// Ros timestamp
		ptime tx;
		ar >> tx;
		timestamp = ros::Time::fromBoost(tx);
	}

	BOOST_SERIALIZATION_SPLIT_MEMBER()
};
*/


/*
class Trajectory : public std::vector<PoseTimestamp>
{
public:

	friend class boost::serialization::access;

	void push_back(const PoseTimestamp &);

	// Return nearest element of provided time
	PoseTimestamp at(const ros::Time&) const;

	PoseTimestamp at(const ptime &t) const;

	PoseTimestamp at(const int idx) const
	{ return std::vector<PoseTimestamp>::at(idx); }

	PoseTimestamp interpolate (const ros::Time&) const;

	PoseTimestamp extrapolate (const ros::Time&) const;

	Trajectory subset(const ros::Time &start, const ros::Time &stop) const;

private:
	uint32_t
	find_lower_bound(const ros::Time&) const;

	uint32_t
	find_lower_bound(const ptime&) const;

	typedef std::vector<PoseTimestamp> Parent;

	template<class Archive>
	inline void serialize(Archive &ar, const unsigned int version)
	{ ar & boost::serialization::base_object<Parent>(*this);}

};
*/


class MeidaiBagDataset;
class MeidaiDataItem : public GenericDataItem
{
public:

	MeidaiDataItem (const MeidaiBagDataset &p, uint64_t idx):
		parent(p), pId(idx)
	{ init(); }

	cv::Mat getImage() const;

	Pose getPose() const;

	Eigen::Vector3d getPosition() const;

	Eigen::Quaterniond getOrientation() const;

	dataItemId getId() const
	{ return pId; }

	ptime getTimestamp() const;

	typedef std::shared_ptr<MeidaiDataItem> Ptr;
	typedef std::shared_ptr<MeidaiDataItem const> ConstPtr;

protected:
	const MeidaiBagDataset &parent;
	dataItemId pId;

	sensor_msgs::Image::ConstPtr bImageMsg;
//	cv_bridge::CvImagePtr imgPtr;
	void init();
};


class MeidaiBagDataset : public GenericDataset
{
public:

	typedef std::shared_ptr<MeidaiBagDataset> Ptr;
	typedef std::shared_ptr<MeidaiBagDataset const> ConstPtr;

	MeidaiBagDataset(
		const std::string &filePath,
//		double startTimeOffsetSecond=0,
//		double mappingDurationSecond=-1,
//		const std::string &calibrationPath=std::string(),
		bool loadPositions=true
	);

/*
	MeidaiBagDataset::Ptr
	subset(const ros::Time &startTime, const ros::Duration &lengthInSecond) const;

	MeidaiBagDataset::Ptr
	subset(const double startTimeOffsetSecond, const double endOffsetFromBeginning) const;
*/

	static MeidaiBagDataset::Ptr load (
		const std::string &filePath,
//		double startTimeOffsetSecond=0,
//		double mappingDurationSecond=-1,
//		const std::string &calibrationPath=std::string(),
		bool loadPositions=true
	);

	void setTimeConstraint(const ptime &start, const ptime &stop);

	void loadPosition();

	virtual ~MeidaiBagDataset();

	size_t size() const;

	size_t sizeAll() const;

	void addCameraParameter(const CameraPinholeParams &c);
	CameraPinholeParams getCameraParameter() const;

	cv::Mat getMask();

	std::string getPath() const
	{ return bagfd->getFileName(); }

	MeidaiDataItem& at(dataItemId i) const;

	const Trajectory& getGnssTrajectory() const
	{ return gnssTrack; }

	const Trajectory& getNdtTrajectory() const
	{ return ndtTrack; }

	const Trajectory& getCompleteCameraTrajectory() const
	{ return cameraTrack; }

	bool isCameraTrajectoryComplete() const;

	GenericDataItem::ConstPtr get(dataItemId i) const;

	GenericDataItem::ConstPtr atDurationSecond (const double second) const;

	bool hasPositioning() const
	{ return !gnssTrack.empty(); }

//	void forceCreateCache (bool resetSubset=false, bool useNdt=true);

	void forceCreateCache (bool useLidar=true, const double startOffset=-1, const double stopOffset=-1);
	void forceCreateCache (bool useLidar=true, const ptime &t1=MIN_TIME, const ptime &t2=MAX_TIME);

	inline void setZoomRatio (float r)
	{ zoomRatio = r; }

	float getZoomRatio () const;

	inline RandomAccessBag::Ptr getVelodyneBag()
	{ return velodyneBag; }

	LidarScanBag::Ptr getLidarScanBag ();

	virtual Trajectory getCameraTrajectory(const ptime timeStart=MIN_TIME, const ptime timeStop=MAX_TIME) const;

	void setLidarParameters (
		const std::string &pvelodyneCalibrationFile,
		const std::string &pmeidaiPCDMapFile,
		const TTransform &plidarToCameraTransform=TTransform::Identity());

	/*
	* Convert time as represented by seconds from start of bag
	*/
	inline ptime timeFromStart(const double seconds) const
	{ return cameraRawBag->timeFromStart(seconds).toBoost(); }

/*
	bool isSubset() const
	{ return isSubset_; }

	inline void getSubsetRange (ptime &beg_, ptime &end_) const
	{ beg_ = subsetBeginTime.toBoost(); end_ = subsetEndTime.toBoost(); }
*/

	virtual
	dataItemId getLowerBound (const ptime &t) const;

	cv::Mat computeImagePreprocessorMask() const;

	// Flag to request preprocessed image
	bool isPreprocessed = true;

	enum CameraTrackSource {
		GNSS,
		NDT,
		ICP
	} cameraTrackSource;


protected:
	static std::string dSetName;

	// Bag Handler
	std::shared_ptr<rosbag::Bag> bagfd;
	RandomAccessBag::Ptr cameraRawBag;
	RandomAccessBag::Ptr gnssBag;
	RandomAccessBag::Ptr velodyneBag;

	const boost::filesystem::path bagPath;

/*
	bool isSubset_ = false;

	ros::Time subsetBeginTime = ros::TIME_MIN,
		subsetEndTime = ros::TIME_MIN;
*/

	CameraPinholeParams cameraParams;

	// Masks for mapping and exposure adjustment
	cv::Mat dashBoardMask;
	cv::Mat exposureMask;
	ImagePreprocessor mPreprocessor;

private:
	void loadCache ();
	void doLoadCache (const std::string &);
	void createTrajectories (ptime startTime=MIN_TIME, ptime stopTime=MAX_TIME, bool useNdt=true);
	void writeCache (const std::string&);

	Trajectory gnssTrack;
	Trajectory ndtTrack;
	Trajectory cameraTrack;

	float zoomRatio = 1.0;

	friend class MeidaiDataItem;

	// To be used when generating Lidar trajectory
	std::string
		velodyneCalibrationFilePath,
		pcdMapFilePath;
	TTransform lidarToCameraTransform;

	// Copy constructor. Reserved for subset, thus made private
	MeidaiBagDataset(const MeidaiBagDataset &cp);

	void prepareBag (const ros::Time &beginTime=ros::TIME_MIN, const ros::Time &stopTime=ros::TIME_MAX);
};


void createTrajectoryFromGnssBag (
	RandomAccessBag &bagsrc,
	Trajectory &trajectory,
	int plane_number=7);

void createTrajectoryFromNDT (
	LidarScanBag &bagsrc,
//	RandomAccessBag &bagsrc,
	Trajectory &resultTrack, const Trajectory &gnssTrack,
//	const std::string &velodyneParamFile,
	const std::string &pcdMapFile);

void createTrajectoryFromNDT2 (
	LidarScanBag &bagsrc,
	Trajectory &resultTrack, const Trajectory &gnssTrack,
	const std::string &pcdMapFile);

void
createTrajectoryFromICP (
	LidarScanBag &bagsrc,
	Trajectory &resultTrack,
	const Trajectory &gnssTrack,
	const std::string &pcdMapFile);


#endif /* _MEIDAIBAG_H_ */
