/*********************************************************************
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
 *   * Neither the name of Willow Garage nor the names of its
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
 *********************************************************************/

/* Author: Ioan Sucan */

#include <moveit/constraint_sampler_manager_loader/constraint_sampler_manager_loader.h>
#include <pluginlib/class_loader.hpp>
#include "rclcpp/rclcpp.hpp"
#include <boost/tokenizer.hpp>
#include <memory>

namespace constraint_sampler_manager_loader
{
static const rclcpp::Logger LOGGER = rclcpp::get_logger("moveit_ros.constraint_sampler_manager_loader");

class ConstraintSamplerManagerLoader::Helper
{
public:
  Helper(const rclcpp::Node::SharedPtr& node, const constraint_samplers::ConstraintSamplerManagerPtr& csm) : node_(node)
  {
    if (node_->has_parameter("constraint_samplers"))
    {
      std::string constraint_samplers;
      node_->get_parameter("constraint_samplers", constraint_samplers);
      try
      {
        constraint_sampler_plugin_loader_ =
            std::make_unique<pluginlib::ClassLoader<constraint_samplers::ConstraintSamplerAllocator>>(
                "moveit_core", "constraint_samplers::ConstraintSamplerAllocator");
      }
      catch (pluginlib::PluginlibException& ex)
      {
        RCLCPP_ERROR(LOGGER, "Exception while creating constraint sampling plugin loader %s", ex.what());
        return;
      }
      boost::char_separator<char> sep(" ");
      boost::tokenizer<boost::char_separator<char>> tok(constraint_samplers, sep);
      for (boost::tokenizer<boost::char_separator<char>>::iterator beg = tok.begin(); beg != tok.end(); ++beg)
      {
        try
        {
          constraint_samplers::ConstraintSamplerAllocatorPtr csa =
              constraint_sampler_plugin_loader_->createUniqueInstance(*beg);
          csm->registerSamplerAllocator(csa);
          RCLCPP_INFO(LOGGER, "Loaded constraint sampling plugin %s", std::string(*beg).c_str());
        }
        catch (pluginlib::PluginlibException& ex)
        {
          RCLCPP_ERROR(LOGGER, "Exception while planning adapter plugin '%s': %s", std::string(*beg).c_str(), ex.what());
        }
      }
    }
  }

private:
  const rclcpp::Node::SharedPtr node_;
  std::unique_ptr<pluginlib::ClassLoader<constraint_samplers::ConstraintSamplerAllocator>>
      constraint_sampler_plugin_loader_;
};
ConstraintSamplerManagerLoader::ConstraintSamplerManagerLoader(
    const rclcpp::Node::SharedPtr& node, const constraint_samplers::ConstraintSamplerManagerPtr& csm)
  : constraint_sampler_manager_(csm ? csm : std::make_shared<constraint_samplers::ConstraintSamplerManager>())
  , impl_(new Helper(node, constraint_sampler_manager_))
{
}
}  // namespace constraint_sampler_manager_loader
