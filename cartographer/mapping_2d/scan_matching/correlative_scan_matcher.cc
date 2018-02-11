/*
 * Copyright 2016 The Cartographer Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cartographer/mapping_2d/scan_matching/correlative_scan_matcher.h"

#include <cmath>

#include "cartographer/common/math.h"

namespace cartographer {
namespace mapping_2d {
namespace scan_matching {

SearchParameters::SearchParameters(const double linear_search_window,
                                   const double angular_search_window,
                                   const sensor::PointCloud& point_cloud,
                                   const double resolution)
    : resolution(resolution) 
{
	// We set this value to something on the order of resolution to make sure that
	// the std::acos() below is defined.
	float max_scan_range = 3.f * resolution;
	for (const Eigen::Vector3f& point : point_cloud) 
	{
		// 获取一个二维点的模（距远点的长度）
		const float range = point.head<2>().norm();
		max_scan_range = std::max(range, max_scan_range);
	}
	
	const double kSafetyMargin = 1. - 1e-3;
	// std::scos 计算反余弦主值
	// 这里的算法不知道有没有出处（很奇怪，感觉应该有一个开根号？） TODO (edward)
	angular_perturbation_step_size = kSafetyMargin * std::acos(1. - common::Pow2(resolution) / (2. * common::Pow2(max_scan_range)));
	  
	// std::ceil
	// Rounds x upward, returning the smallest integral value that is not less than x.
	// 获取整个search过程中角度转动的 step 数
	num_angular_perturbations = std::ceil(angular_search_window / angular_perturbation_step_size);
	num_scans = 2 * num_angular_perturbations + 1;

	const int num_linear_perturbations = std::ceil(linear_search_window / resolution);
	linear_bounds.reserve(num_scans);
	for (int i = 0; i != num_scans; ++i) 
	{
		linear_bounds.push_back(
			LinearBounds{-num_linear_perturbations, num_linear_perturbations,
						-num_linear_perturbations, num_linear_perturbations});
	}
}

// 这个函数用于测试，可以不考虑
SearchParameters::SearchParameters(const int num_linear_perturbations,
                                   const int num_angular_perturbations,
                                   const double angular_perturbation_step_size,
                                   const double resolution)
    : num_angular_perturbations(num_angular_perturbations),
      angular_perturbation_step_size(angular_perturbation_step_size),
      resolution(resolution),
      num_scans(2 * num_angular_perturbations + 1) {
  linear_bounds.reserve(num_scans);
  for (int i = 0; i != num_scans; ++i) {
    linear_bounds.push_back(
        LinearBounds{-num_linear_perturbations, num_linear_perturbations,
                     -num_linear_perturbations, num_linear_perturbations});
  }
}

void SearchParameters::ShrinkToFit(const std::vector<DiscreteScan>& scans,
                                   const CellLimits& cell_limits) 
{
	CHECK_EQ(scans.size(), num_scans);
	CHECK_EQ(linear_bounds.size(), num_scans);
	for (int i = 0; i != num_scans; ++i) 
	{
		Eigen::Array2i min_bound = Eigen::Array2i::Zero();
		Eigen::Array2i max_bound = Eigen::Array2i::Zero();
		for (const Eigen::Array2i& xy_index : scans[i]) 
		{
			min_bound = min_bound.min(-xy_index);
			max_bound = max_bound.max(
				Eigen::Array2i(cell_limits.num_x_cells - 1,cell_limits.num_y_cells - 1) - xy_index);
		}
		linear_bounds[i].min_x = std::max(linear_bounds[i].min_x, min_bound.x());
		linear_bounds[i].max_x = std::min(linear_bounds[i].max_x, max_bound.x());
		linear_bounds[i].min_y = std::max(linear_bounds[i].min_y, min_bound.y());
		linear_bounds[i].max_y = std::min(linear_bounds[i].max_y, max_bound.y());
	}
}

std::vector<sensor::PointCloud> GenerateRotatedScans(
    const sensor::PointCloud& point_cloud,
    const SearchParameters& search_parameters) 
{
	std::vector<sensor::PointCloud> rotated_scans;
	rotated_scans.reserve(search_parameters.num_scans);

	// 角度从最小值逐渐加到最大值，生成 num_scans 个旋转过的点云
	double delta_theta = -search_parameters.num_angular_perturbations *
						search_parameters.angular_perturbation_step_size;
						
	for (	int scan_index = 0; 
			scan_index < search_parameters.num_scans;
			++scan_index,
				delta_theta += search_parameters.angular_perturbation_step_size) 
	{
		rotated_scans.push_back(sensor::TransformPointCloud(
			point_cloud, transform::Rigid3f::Rotation(Eigen::AngleAxisf(
                         delta_theta, Eigen::Vector3f::UnitZ()))));
	}
	return rotated_scans;
}

// 将传入的点云信息离散化
// 每一个 point cloud 对应一个离散点vector
std::vector<DiscreteScan> DiscretizeScans(
		const MapLimits& map_limits, 
		const std::vector<sensor::PointCloud>& scans,
		const Eigen::Translation2f& initial_translation) 
{
	std::vector<DiscreteScan> discrete_scans;
	discrete_scans.reserve(scans.size());
	for (const sensor::PointCloud& scan : scans) 
	{
		discrete_scans.emplace_back();
		discrete_scans.back().reserve(scan.size());
		for (const Eigen::Vector3f& point : scan) 
		{
			const Eigen::Vector2f translated_point =
				Eigen::Affine2f(initial_translation) * point.head<2>();
			discrete_scans.back().push_back(map_limits.GetCellIndex(translated_point));
		}
	}
	return discrete_scans;
}

}  // namespace scan_matching
}  // namespace mapping_2d
}  // namespace cartographer
