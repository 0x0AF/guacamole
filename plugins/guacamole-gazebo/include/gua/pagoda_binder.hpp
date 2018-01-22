#ifndef GUACAMOLE_PAGODA_H
#define GUACAMOLE_PAGODA_H

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_set>

#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <boost/enable_shared_from_this.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>

#include <gazebo/gazebo_client.hh>
#include <gazebo/transport/transport.hh>

#include <gazebo/msgs/msgs.hh>

#include <gazebo/common/Assert.hh>
#include <gazebo/common/Console.hh>
#include <gazebo/common/Exception.hh>

#include <gazebo/transport/Node.hh>
#include <gazebo/transport/TransportIface.hh>

#include <gazebo/msgs/msgs.hh>
#include <gazebo/transport/TransportTypes.hh>

#include <gua/node/TransformNode.hpp>
#include <gua/renderer/MaterialLoader.hpp>
#include <gua/renderer/TriMeshLoader.hpp>
#include <gua/scenegraph/SceneGraph.hpp>

#include "pagoda/pagoda_log.hpp"
#include "pagoda/pagoda_scene.hpp"

class PagodaBinder
{
  public:
    PagodaBinder();
    ~PagodaBinder();

    void bind_scene_graph(gua::SceneGraph *sceneGraph);
    void bind_transport_layer(int argc, char **argv);

    void pre_render();

  private:
    std::thread _worker;
    std::mutex _worker_mutex;
    std::condition_variable _worker_cv;

    PagodaLog _log;
    PagodaScene _scene;

    void _connect_to_transport_layer(int argc, char **argv);
    void _halt_transport_layer();

    void callback_world(ConstWorldStatisticsPtr &_msg);
    void callback_material(ConstMaterialPtr &_msg);
    void callback_skeleton_pose_info(ConstPoseAnimationPtr &ptr);
    void callback_model_info(ConstModelPtr &ptr);
    void callback_pose_info(ConstPosesStampedPtr &ptr);
    void callback_request(ConstRequestPtr &ptr);
    void callback_response(ConstResponsePtr &ptr);
};

#endif // GUACAMOLE_PAGODA_H