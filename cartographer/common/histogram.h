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

#ifndef CARTOGRAPHER_COMMON_HISTOGRAM_H_
#define CARTOGRAPHER_COMMON_HISTOGRAM_H_

#include <string>
#include <vector>

#include "cartographer/common/port.h"

namespace cartographer {
namespace common {

// 直方图
// 是一种对数据分布情况的图形表示，是一种二维统计图表，它的两个坐标分别是统计样本和该样本对应的某个属性的度量。
class Histogram {
 public:
  void Add(float value);
  std::string ToString(int buckets) const;

 private:
  std::vector<float> values_;
};

}  // namespace common
}  // namespace cartographer

#endif  // CARTOGRAPHER_COMMON_HISTOGRAM_H_
