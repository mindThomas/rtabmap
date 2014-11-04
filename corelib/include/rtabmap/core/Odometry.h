/*
Copyright (c) 2010-2014, Mathieu Labbe - IntRoLab - Universite de Sherbrooke
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Universite de Sherbrooke nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef ODOMETRY_H_
#define ODOMETRY_H_

#include <rtabmap/core/RtabmapExp.h>

#include <rtabmap/utilite/UThread.h>
#include <rtabmap/utilite/UEventsHandler.h>
#include <rtabmap/utilite/UEvent.h>
#include <rtabmap/utilite/UMutex.h>
#include <rtabmap/utilite/USemaphore.h>

#include <rtabmap/core/Parameters.h>

#include <rtabmap/core/SensorData.h>

#include <opencv2/opencv.hpp>

#include <pcl/common/eigen.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>

class UTimer;

namespace rtabmap {

class Feature2D;

class RTABMAP_EXP Odometry
{
public:
	virtual ~Odometry() {}
	Transform process(SensorData & data, int * quality = 0, int * features = 0, int * localMapSize = 0);
	virtual void reset(const Transform & initialPose = Transform::getIdentity());

	bool isLargeEnoughTransform(const Transform & transform);

	//getters
	const Transform & getPose() const {return _pose;}
	int getMaxFeatures() const  {return _maxFeatures;}
	const std::string & getRoiRatios() const {return _roiRatios;}
	int getMinInliers() const {return _minInliers;}
	float getInlierDistance() const {return _inlierDistance;}
	int getIterations() const {return _iterations;}
	int getRefineIterations() const {return _refineIterations;}
	float getFeaturesRatio() const {return _featuresRatio;}
	float getMaxDepth() const {return _maxDepth;}
	float geLinearUpdate() const {return _linearUpdate;}
	float getAngularUpdate() const {return _angularUpdate;}

private:
	virtual Transform computeTransform(const SensorData & image, int * quality = 0, int * features = 0, int * localMapSize = 0) = 0;

private:
	int _maxFeatures;
	std::string _roiRatios;
	int _minInliers;
	float _inlierDistance;
	int _iterations;
	int _refineIterations;
	float _featuresRatio;
	float _maxDepth;
	float _linearUpdate;
	float _angularUpdate;
	int _resetCountdown;
	Transform _pose;
	int _resetCurrentCount;

protected:
	Odometry(const rtabmap::ParametersMap & parameters);
};

class Memory;

class RTABMAP_EXP OdometryBOW : public Odometry
{
public:
	OdometryBOW(const rtabmap::ParametersMap & parameters = rtabmap::ParametersMap());
	virtual ~OdometryBOW();

	virtual void reset(const Transform & initialPose = Transform::getIdentity());
	const std::multimap<int, pcl::PointXYZ> & getLocalMap() const {return localMap_;}
	const Memory * getMemory() const {return _memory;}

private:
	virtual Transform computeTransform(const SensorData & image, int * quality = 0, int * features = 0, int * localMapSize = 0);

private:
	//Parameters
	int _localHistoryMaxSize;

	Memory * _memory;
	std::multimap<int, pcl::PointXYZ> localMap_;
};

class RTABMAP_EXP OdometryOpticalFlow : public Odometry
{
public:
	OdometryOpticalFlow(const rtabmap::ParametersMap & parameters = rtabmap::ParametersMap());
	virtual ~OdometryOpticalFlow();

	virtual void reset(const Transform & initialPose = Transform::getIdentity());

	const cv::Mat & getLastFrame() const {return lastFrame_;}
	const std::vector<cv::Point2f> & getLastCorners() const {return lastCorners_;}
	const pcl::PointCloud<pcl::PointXYZ>::Ptr & getLastCorners3D() const {return lastCorners3D_;}

	cv::Mat imgMatches_;

private:
	virtual Transform computeTransform(const SensorData & image, int * quality = 0, int * features = 0, int * localMapSize = 0);
	Transform computeTransformStereo(const SensorData & image, int * quality, int * features);
	Transform computeTransformRGBD(const SensorData & image, int * quality, int * features);
private:
	//Parameters:
	int flowWinSize_;
	int flowIterations_;
	double flowEps_;
	int flowMaxLevel_;
	float stereoMaxSlope_;

	int subPixWinSize_;
	int subPixIterations_;
	double subPixEps_;

	Feature2D * feature2D_;

	cv::Mat lastFrame_;
	cv::Mat lastRightFrame_;
	std::vector<cv::Point2f> lastCorners_;
	pcl::PointCloud<pcl::PointXYZ>::Ptr lastCorners3D_;
	Transform savedLastRefFrameTransform_;
};

class RTABMAP_EXP OdometryICP : public Odometry
{
public:
	OdometryICP(int decimation = 4,
			float voxelSize = 0.005f,
			int samples = 0,
			float maxCorrespondenceDistance = 0.05f,
			int maxIterations = 30,
			float maxFitness = 0.01f,
			bool pointToPlane = true,
			const ParametersMap & odometryParameter = rtabmap::ParametersMap());
	virtual void reset(const Transform & initialPose = Transform::getIdentity());

private:
	virtual Transform computeTransform(const SensorData & image, int * quality = 0, int * features = 0, int * localMapSize = 0);

private:
	int _decimation;
	float _voxelSize;
	float _samples;
	float _maxCorrespondenceDistance;
	int	_maxIterations;
	float _maxFitness;
	bool _pointToPlane;

	pcl::PointCloud<pcl::PointNormal>::Ptr _previousCloudNormal; // for point ot plane
	pcl::PointCloud<pcl::PointXYZ>::Ptr _previousCloud; // for point to point
};

class RTABMAP_EXP OdometryThread : public UThread, public UEventsHandler {
public:
	// take ownership of Odometry
	OdometryThread(Odometry * odometry);
	virtual ~OdometryThread();

protected:
	virtual void handleEvent(UEvent * event);

private:
	void mainLoopKill();

	//============================================================
	// MAIN LOOP
	//============================================================
	void mainLoop();
	void addData(const SensorData & data);
	void getData(SensorData & data);

private:
	USemaphore _dataAdded;
	UMutex _dataMutex;
	SensorData _dataBuffer;
	Odometry * _odometry;
	bool _resetOdometry;
};

} /* namespace rtabmap */
#endif /* ODOMETRY_H_ */
