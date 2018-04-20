#ifndef GUACAMOLE_PAGODA_LIGHT_H
#define GUACAMOLE_PAGODA_LIGHT_H

#include <gazebo/common/Console.hh>
#include <gazebo/common/common.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/rendering/Visual.hh>
#include <gua/node/Node.hpp>
#include <gua/node/TransformNode.hpp>
#include <gua/nrp/platform.hpp>
#include <gua/renderer/TriMeshLoader.hpp>
#include <memory>
namespace gua
{
namespace nrp
{
class PagodaLight;

typedef std::shared_ptr<PagodaLight> ptr_light;

class GUA_NRP_DLL PagodaLight
{
  public:
    PagodaLight(const std::string &name, node::Node *root_node);
    ~PagodaLight();

    void load_from_msg(const boost::shared_ptr<const gazebo::msgs::Light> &msg);

  private:
    void set_pose(const gazebo::math::Pose &pose);

    std::shared_ptr<gua::node::LightNode> _node;
    double _scale;
};
}
}
#endif // GUACAMOLE_PAGODA_LIGHT_H
