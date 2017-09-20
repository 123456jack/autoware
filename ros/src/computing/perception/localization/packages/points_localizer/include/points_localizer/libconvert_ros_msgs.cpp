/*
 *  Copyright (c) 2017, Tier IV, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Autoware nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "libconvert_ros_msgs.h"

#include <tf/tf.h>

#include <sensor_msgs/Imu.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <geometry_msgs/TwistStamped.h>

#include "libdata_structs.h"


geometry_msgs::PoseStamped convertToROSMsg(const std_msgs::Header& header, const Pose& pose)
{
  tf::Quaternion q;
  q.setRPY(pose.roll, pose.pitch, pose.yaw);

  geometry_msgs::PoseStamped msg;
  msg.header = header;
  msg.pose.position.x = pose.x;
  msg.pose.position.y = pose.y;
  msg.pose.position.z = pose.z;
  msg.pose.orientation.x = q.x();
  msg.pose.orientation.y = q.y();
  msg.pose.orientation.z = q.z();
  msg.pose.orientation.w = q.w();
  return msg;
}

geometry_msgs::PoseStamped convertToROSMsg(const std_msgs::Header& header, const Pose& pose, const tf::Transform& local_transform)
{
  tf::Quaternion q;
  q.setRPY(pose.roll, pose.pitch, pose.yaw);

  tf::Vector3 v(pose.x, pose.y, pose.z);
  tf::Transform transform(q, v);

  geometry_msgs::PoseStamped msg;
  msg.header = header;
  msg.pose.position.x = (local_transform * transform).getOrigin().getX();
  msg.pose.position.y = (local_transform * transform).getOrigin().getY();
  msg.pose.position.z = (local_transform * transform).getOrigin().getZ();
  msg.pose.orientation.x = (local_transform * transform).getRotation().x();
  msg.pose.orientation.y = (local_transform * transform).getRotation().y();
  msg.pose.orientation.z = (local_transform * transform).getRotation().z();
  msg.pose.orientation.w = (local_transform * transform).getRotation().w();
  return msg;
}

geometry_msgs::TwistStamped convertToROSMsg(const std_msgs::Header& header, const Velocity& velocity)
{
  geometry_msgs::TwistStamped msg;
  msg.header = header;
  // msg.twist.linear.x = std::sqrt(velocity.linear.x*velocity.linear.x + velocity.linear.y*velocity.linear.y + velocity.linear.z*velocity.linear.z);
  // msg.twist.linear.y = 0.0;
  // msg.twist.linear.z = 0.0;
  // msg.twist.angular.x = 0.0;
  // msg.twist.angular.y = 0.0;
  // msg.twist.angular.z = velocity.angular.z;
  msg.twist.linear.x = velocity.linear.x;
  msg.twist.linear.y = velocity.linear.y;
  msg.twist.linear.z = velocity.linear.z;
  msg.twist.angular.x = 0.0;
  msg.twist.angular.y = 0.0;
  msg.twist.angular.z = velocity.angular.z;
  return msg;
}

Pose convertFromROSMsg(const geometry_msgs::Pose& msg)
{
  double roll, pitch, yaw;
  tf::Quaternion orientation;
  tf::quaternionMsgToTF(msg.orientation, orientation);
  tf::Matrix3x3(orientation).getRPY(roll, pitch, yaw);

  Pose pose;
  pose.x = msg.position.x;
  pose.y = msg.position.y;
  pose.z = msg.position.z;
  pose.roll = roll;
  pose.pitch = pitch;
  pose.yaw = yaw;

  return pose;
}

Pose convertFromROSMsg(const geometry_msgs::PoseStamped& msg)
{
    return convertFromROSMsg(msg.pose);
}

Pose convertFromROSMsg(const geometry_msgs::PoseWithCovarianceStamped& msg)
{
    return convertFromROSMsg(msg.pose.pose);
}


Velocity convertFromROSMsg(const geometry_msgs::TwistStamped& msg)
{
  Velocity velocity;
  velocity.linear.x = msg.twist.linear.x;
  velocity.linear.y = msg.twist.linear.y;
  velocity.linear.z = msg.twist.linear.z;
  velocity.angular.x = msg.twist.angular.x;
  velocity.angular.y = msg.twist.angular.y;
  velocity.angular.z = msg.twist.angular.z;

  return velocity;
}

Imu convertFromROSMsg(const sensor_msgs::Imu& msg)
{
  double roll, pitch, yaw;
  tf::Quaternion orientation;
  tf::quaternionMsgToTF(msg.orientation, orientation);
  tf::Matrix3x3(orientation).getRPY(roll, pitch, yaw);

  Imu imu;
  imu.roll = roll;
  imu.pitch = pitch;
  imu.yaw = yaw;
  imu.angular_velocity.x = msg.angular_velocity.x;
  imu.angular_velocity.y = msg.angular_velocity.y;
  imu.angular_velocity.z = msg.angular_velocity.z;
  imu.linear_accel.x = msg.linear_acceleration.x;
  imu.linear_accel.y = msg.linear_acceleration.y;
  imu.linear_accel.z = msg.linear_acceleration.z;

  return imu;
}
