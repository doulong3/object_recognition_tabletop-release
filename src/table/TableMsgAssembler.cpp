/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <fstream>
#include <iostream>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>

#include <ecto/ecto.hpp>

#include <pcl/ModelCoefficients.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/PointIndices.h>

#include <sensor_msgs/Image.h>
#include <sensor_msgs/PointCloud.h>
#include <tf/transform_listener.h>
#include <tf/transform_broadcaster.h>

#include <object_recognition_msgs/Table.h>
#include <object_recognition_msgs/TableArray.h>

#include <tabletop_object_detector/marker_generator.h>
#include <object_recognition_core/common/pose_result.h>

#include <tf/LinearMath/Transform.h>

using object_recognition_core::common::PoseResult;

using ecto::tendrils;

namespace tabletop
{
  /** Ecto implementation of a module that takes
   *
   */
  struct TableMsgAssembler
  {
    static void
    declare_params(ecto::tendrils& params)
    {
    }

    static void
    declare_io(const tendrils& params, tendrils& inputs, tendrils& outputs)
    {
      inputs.declare(&TableMsgAssembler::image_message_, "image_message", "the image message to get the header").required(
          true);
      inputs.declare(&TableMsgAssembler::pose_results_, "pose_results", "The results of object recognition").required(
          true);
      inputs.declare(&TableMsgAssembler::clouds_, "clouds", "Some samples from the table.").required(true);
      inputs.declare(&TableMsgAssembler::clouds_hull_, "clouds_hull", "The hull of the samples.").required(true);

      outputs.declare<object_recognition_msgs::TableArrayConstPtr>("table_array_msg", "The message for the found tables");
    }

    void
    configure(const tendrils& params, const tendrils& inputs, const tendrils& outputs)
    {
      flatten_table_ = false;
    }

    /** Get the 2d keypoints and figure out their 3D position from the depth map
     * @param inputs
     * @param outputs
     * @return
     */
    int
    process(const tendrils& inputs, const tendrils& outputs)
    {
      typedef pcl::PointXYZ Point;

      std::string frame_id;
      if (*image_message_)
        frame_id = (*image_message_)->header.frame_id;

      object_recognition_msgs::TableArray table_array_msg;

      std_msgs::Header message_header;
      message_header.frame_id = frame_id;

      for (size_t table_index = 0; table_index < pose_results_->size(); ++table_index)
      {
        const PoseResult & pose_result = (*pose_results_)[table_index];
        pcl::PointCloud<pcl::PointXYZ>::Ptr & cloud = (*clouds_)[table_index];
        pcl::PointCloud<pcl::PointXYZ>::Ptr & cloud_hull = (*clouds_hull_)[table_index];

        sensor_msgs::PointCloud table_points;
        sensor_msgs::PointCloud table_hull_points;
        tf::Transform table_plane_trans = getPlaneTransform(pose_result);

        object_recognition_msgs::Table table;

        table_points.header.frame_id = frame_id;
        table_hull_points.header.frame_id = frame_id;
        cloud->header.frame_id = frame_id;
        cloud_hull->header.frame_id = frame_id;

        if (!flatten_table_)
        {
          // --- [ Take the points projected on the table and transform them into the PointCloud message
          //  while also transforming them into the table's coordinate system
          if (!getPlanePoints<Point>(*cloud, table_plane_trans, table_points))
          {
            //response.result = response.OTHER_ERROR;
            return ecto::OK;
          }

          // ---[ Create the table message
          // TODO use the original cloud header
          table = getTable<sensor_msgs::PointCloud>(message_header, table_plane_trans, table_points);

          // ---[ Convert the convex hull points to table frame
          if (!getPlanePoints<Point>(*cloud_hull, table_plane_trans, table_hull_points))
          {
            //response.result = response.OTHER_ERROR;
            return ecto::OK;
          }
        }
        if (flatten_table_)
        {
          // if flattening the table, find the center of the convex hull and move the table frame there
          tf::Vector3 flat_table_pos;
          double avg_x, avg_y, avg_z;
          avg_x = avg_y = avg_z = 0;
          for (size_t i = 0; i < cloud->points.size(); i++)
          {
            avg_x += cloud->points[i].x;
            avg_y += cloud->points[i].y;
            avg_z += cloud->points[i].z;
          }
          avg_x /= cloud->points.size();
          avg_y /= cloud->points.size();
          avg_z /= cloud->points.size();

          // place the new table frame in the center of the convex hull
          flat_table_pos[0] = avg_x;
          flat_table_pos[1] = avg_y;
          flat_table_pos[2] = avg_z;
          table_plane_trans.setOrigin(flat_table_pos);

          // --- [ Take the points projected on the table and transform them into the PointCloud message
          //  while also transforming them into the flat table's coordinate system
          sensor_msgs::PointCloud flat_table_points;
          if (!getPlanePoints<Point>(*cloud, table_plane_trans, flat_table_points))
          {
            //TODOresponse.result = response.OTHER_ERROR;
            return ecto::OK;
          }

          // ---[ Create the table message
          // TODO use the original cloud header
          table = getTable<sensor_msgs::PointCloud>(message_header, table_plane_trans, flat_table_points);

          // ---[ Convert the convex hull points to flat table frame
          if (!getPlanePoints<Point>(*cloud_hull, table_plane_trans, table_hull_points))
          {
            return ecto::OK;
          }
        }

        // ---[ Add the convex hull as a triangle mesh to the Table message
        addConvexHullTable<sensor_msgs::PointCloud>(table, table_hull_points, flatten_table_);
        table_array_msg.tables.push_back(table);
      }

      outputs["table_array_msg"] << object_recognition_msgs::TableArrayConstPtr(new object_recognition_msgs::TableArray(table_array_msg));

      return ecto::OK;
    }
  private:
    /*! Assumes plane coefficients are of the form ax+by+cz+d=0, normalized */
    tf::Transform
    getPlaneTransform(const PoseResult & pose_result)
    {
      Eigen::Vector3f translation = pose_result.T<Eigen::Vector3f>();
      Eigen::Matrix3f rotation = pose_result.R<Eigen::Matrix3f>();

      tf::Vector3 position_tf(translation[0], translation[1], translation[2]);
      tf::Matrix3x3 rotation_tf(rotation.coeff(0, 0), rotation.coeff(0, 1), rotation.coeff(0, 2), rotation.coeff(1, 0),
          rotation.coeff(1, 1), rotation.coeff(1, 2), rotation.coeff(2, 0), rotation.coeff(2, 1),
          rotation.coeff(2, 2));

      tf::Quaternion orientation;
      rotation_tf.getRotation(orientation);

      return tf::Transform(orientation, position_tf);
    }

    template<typename PointT>
    bool
    getPlanePoints(const pcl::PointCloud<PointT> &table, const tf::Transform& table_plane_trans,
                   sensor_msgs::PointCloud &table_points)
    {
      // Prepare the output
      table_points.header = table.header;
      table_points.points.resize(table.points.size());
      for (size_t i = 0; i < table.points.size(); ++i)
      {
        table_points.points[i].x = table.points[i].x;
        table_points.points[i].y = table.points[i].y;
        table_points.points[i].z = table.points[i].z;
      }

      // Transform the data
      tf::TransformListener listener;
      tf::StampedTransform table_pose_frame(table_plane_trans, table.header.stamp, table.header.frame_id,
                                            "table_frame");
      listener.setTransform(table_pose_frame);
      std::string error_msg;
      if (!listener.canTransform("table_frame", table_points.header.frame_id, table_points.header.stamp, &error_msg))
      {
        return false;
      }
      try
      {
        listener.transformPointCloud("table_frame", table_points, table_points);
      } catch (tf::TransformException &ex)
      {
        return false;
      }
      table_points.header.stamp = table.header.stamp;
      table_points.header.frame_id = "table_frame";
      return true;
    }

    template<class PointCloudType>
    void
    addConvexHullTable(object_recognition_msgs::Table &table, const PointCloudType &convex_hull, bool flatten_table)
    {
      //create a triangle mesh out of the convex hull points and add it to the table message
      //table.convex_hull.type = table.convex_hull.MESH;
      for (size_t i = 0; i < convex_hull.points.size(); i++)
      {
        geometry_msgs::Point vertex;
        vertex.x = convex_hull.points[i].x;
        vertex.y = convex_hull.points[i].y;
        if (flatten_table)
          vertex.z = 0;
        else
          vertex.z = convex_hull.points[i].z;
        table.convex_hull.vertices.push_back(vertex);

        if (i == 0 || i == convex_hull.points.size() - 1)
          continue;

        shape_msgs::MeshTriangle tri;
        tri.vertex_indices[0] = 0;
        tri.vertex_indices[1] = i;
        tri.vertex_indices[2] = i + 1;
        table.convex_hull.triangles.push_back(tri);
      }
    }

    template<class PointCloudType>
    object_recognition_msgs::Table
    getTable(const std_msgs::Header &cloud_header, const tf::Transform &table_plane_trans,
             const PointCloudType &table_points)
    {
      object_recognition_msgs::Table table;

      //get the extents of the table
      if (!table_points.points.empty())
      {
        table.x_min = table_points.points[0].x;
        table.x_max = table_points.points[0].x;
        table.y_min = table_points.points[0].y;
        table.y_max = table_points.points[0].y;
      }
      for (size_t i = 1; i < table_points.points.size(); ++i)
      {
        if (table_points.points[i].x < table.x_min && table_points.points[i].x > -3.0)
          table.x_min = table_points.points[i].x;
        if (table_points.points[i].x > table.x_max && table_points.points[i].x < 3.0)
          table.x_max = table_points.points[i].x;
        if (table_points.points[i].y < table.y_min && table_points.points[i].y > -3.0)
          table.y_min = table_points.points[i].y;
        if (table_points.points[i].y > table.y_max && table_points.points[i].y < 3.0)
          table.y_max = table_points.points[i].y;
      }

      geometry_msgs::Pose table_pose;
      tf::poseTFToMsg(table_plane_trans, table_pose);
      table.pose.pose = table_pose;
      table.pose.header = cloud_header;

      return table;
    }

    /** The distance used as a threshold when finding a plane */
    bool flatten_table_;

    /** flag indicating whether we run in debug mode */
    ecto::spore<std::vector<pcl::PointCloud<pcl::PointXYZ>::Ptr> > clouds_, clouds_hull_;
    /** The vertical direction */
    ecto::spore<Eigen::Vector3f> up_direction_;

    //! The current marker being published
    ecto::spore<sensor_msgs::ImageConstPtr> image_message_;

    ecto::spore<std::vector<PoseResult> > pose_results_;

    ecto::spore<object_recognition_msgs::TableArrayConstPtr> table_array_msg_;
  };
}

ECTO_CELL(tabletop_table, tabletop::TableMsgAssembler, "TableMsgAssembler",
          "Given a point cloud, find  a potential table.");
