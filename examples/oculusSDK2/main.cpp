/******************************************************************************
 * guacamole - delicious VR                                                   *
 *                                                                            *
 * Copyright: (c) 2011-2013 Bauhaus-Universität Weimar                        *
 * Contact:   felix.lauer@uni-weimar.de / simon.schneegans@uni-weimar.de      *
 *                                                                            *
 * This program is free software: you can redistribute it and/or modify it    *
 * under the terms of the GNU General Public License as published by the Free *
 * Software Foundation, either version 3 of the License, or (at your option)  *
 * any later version.                                                         *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
 * for more details.                                                          *
 *                                                                            *
 * You should have received a copy of the GNU General Public License along    *
 * with this program. If not, see <http://www.gnu.org/licenses/>.             *
 *                                                                            *
 ******************************************************************************/

#include <gua/guacamole.hpp>
#include <gua/renderer/TriMeshLoader.hpp>
#include <gua/utils/Logger.hpp>
#include <gua/OculusSDK2Window.hpp>

#include <OVR.h>
#include <Extras/OVR_Math.h>
const std::string geometry("data/objects/monkey.obj");
// const std::string geometry("data/objects/cube.obj");

std::vector<std::shared_ptr<gua::node::TransformNode>> add_lights(gua::SceneGraph& graph,
                                                  int count) {

  std::vector<std::shared_ptr<gua::node::TransformNode>> lights(count);

  for (int i(0); i < count; ++i) {
    scm::math::vec3 randdir(gua::math::random::get(-1.f, 1.f),
                            gua::math::random::get(-1.f, 1.f),
                            gua::math::random::get(-1.f, 1.f));
    scm::math::normalize(randdir);

    gua::TriMeshLoader loader;
    auto sphere_geometry(
      loader.create_geometry_from_file(
      "sphere" + gua::string_utils::to_string(i),
      "data/objects/light_sphere.obj"
    ));

    sphere_geometry->scale(0.04, 0.04, 0.04);

    lights[i] = graph.add_node("/", std::make_shared<gua::node::TransformNode>("light" + gua::string_utils::to_string(i)));
    lights[i]->add_child(sphere_geometry);
    lights[i]->translate(randdir[0], randdir[1], randdir[2]);

    auto light = lights[i]->add_child(std::make_shared<gua::node::LightNode>("light"));
    light->data.set_type(gua::node::LightNode::Type::POINT);
    light->data.set_color(gua::utils::Color3f::random());
  }

  return lights;
}

void setup_scene(gua::SceneGraph& graph,
                 std::shared_ptr<gua::node::Node> const& root_monkey,
                 int depth_count) {

  if (depth_count > 0) {
    gua::TriMeshLoader loader;

    float offset(2.f);
    std::vector<gua::math::vec3> directions = {
      gua::math::vec3(0, offset, 0),
      gua::math::vec3(0, -offset, 0),
      gua::math::vec3(offset, 0, 0),
      gua::math::vec3(-offset, 0, 0),
      gua::math::vec3(0, 0, offset),
      gua::math::vec3(0, 0, -offset)
    };

    for (auto direction: directions) {
      auto monkey_geometry(loader.create_geometry_from_file(
        "monkey",
        geometry
      ));

      auto monkey = root_monkey->add_child(monkey_geometry);
      monkey->scale(0.5, 0.5, 0.5);
      monkey->translate(direction[0], direction[1], direction[2]);

      setup_scene(graph, monkey, depth_count - 1);
    }
  }
}
/*
OVR::SensorFusion* init_oculus() {
  OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_All));
  OVR::DeviceManager* device_manager  = OVR::DeviceManager::Create();
  OVR::SensorDevice*  sensor_device   = device_manager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();
  if (sensor_device) {
    OVR::SensorFusion* sensor_fusion = new OVR::SensorFusion();
    sensor_fusion->AttachToSensor(sensor_device);
    return sensor_fusion;
  }
  return nullptr;
}
*/

void init_oculus() {
  ovr_Initialize();
}

void shutdown_oculus() {
  ovr_Shutdown();
}

/*
gua::math::mat4 const get_oculus_transform(OVR::SensorFusion* sensor) {
  OVR::Quatf orient = sensor->GetPredictedOrientation();
  OVR::Matrix4f mat(orient.Inverted());
  return gua::math::mat4( mat.M[0][0], mat.M[0][1], mat.M[0][2], mat.M[0][3],
                          mat.M[1][0], mat.M[1][1], mat.M[1][2], mat.M[1][3],
                          mat.M[2][0], mat.M[2][1], mat.M[2][2], mat.M[2][3],
                          mat.M[3][0], mat.M[3][1], mat.M[3][2], mat.M[3][3]);

  return gua::math::identity();
}
*/

gua::math::mat4 const get_oculus_transform( ovrHmd const& hmd) {
  auto frameTiming = ovrHmd_BeginFrameTiming(hmd, 0);
ovrTrackingState ts = ovrHmd_GetTrackingState(hmd, frameTiming.ScanoutMidpointSeconds);

if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)) {
  // The cpp compatibility layer is used to convert ovrPosef to Posef (see OVR_Math.h)
  auto pose = ts.HeadPose.ThePose;

  scm::math::quat<double> rot_quat(pose.Orientation.x,
                           pose.Orientation.y,
                           pose.Orientation.z,
                           pose.Orientation.w);

  ovrHmd_EndFrameTiming(hmd);

  return scm::math::inverse(rot_quat.to_matrix());
}



  ovrHmd_EndFrameTiming(hmd);
  return gua::math::mat4::identity();
}

int main(int argc, char** argv) {

  // initialize guacamole
  gua::init(argc, argv);

  // initialize Oculus SDK
  //OVR::SensorFusion* oculus_sensor = init_oculus();

  init_oculus();
  ovrHmd hmd = ovrHmd_Create(0);

  if(!hmd) {
    std::cout << "Unable to create HMD: " << ovrHmd_GetLastError(NULL);
    shutdown_oculus();
    std::cout << "Exiting..\n";
    exit(-1);
  }


  bool tracking_configured = ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);

  if( !tracking_configured ) {
    ovrHmd_Destroy(hmd);
    hmd = NULL;
    std::cout << "The oculus device does not support demanded tracking features\n";
    shutdown_oculus();
    std::cout << "Exiting..\n";
    exit(-1); 
  }


  // setup scene
  gua::SceneGraph graph("main_scenegraph");

  gua::TriMeshLoader loader;

  auto monkey_geometry(loader.create_geometry_from_file(
    "root_ape",
    geometry
  ));

  auto root_monkey = graph.add_node("/", monkey_geometry);
  root_monkey->scale(1.5, 1.5, 1.5);

  // depth    monkey    cube          car
  // 1        14.084      56    3.619.000 Vertices  /      7 draw calls
  // 2        74.444     296   19.129.000 Vertices  /     37 draw calls
  // 3       436.604   1.736  112.189.000 Vertices  /    217 draw calls
  // 4     2.609.564  10.376              Vertices  /  1.297 draw calls
  // 5    15.647.324  62.216              Vertices  /  7.777 draw calls
  // 6    93.873.884 373.256              Vertices  / 46.657 draw calls
  setup_scene(graph, root_monkey, 4);

  auto lights = add_lights(graph, 50);

  auto nav = graph.add_node<gua::node::TransformNode>("/", "nav");
  nav->translate(0.0, 0.0, 2.0);

  // setup rendering pipeline and window
  auto resolution = gua::math::vec2ui(1280, 800);

  auto resolve_pass = std::make_shared<gua::ResolvePassDescription>();
  //resolve_pass->background_mode(gua::ResolvePassDescription::BackgroundMode::QUAD_TEXTURE);
  resolve_pass->tone_mapping_exposure(1.0f);

  auto camera = graph.add_node<gua::node::CameraNode>("/nav", "cam");

  //camera->translate(0, 0, 2.0);
  camera->config.set_resolution(resolution);
  camera->config.set_left_screen_path("/nav/cam/left_screen");
  camera->config.set_right_screen_path("/nav/cam/right_screen");
  camera->config.set_scene_graph_name("main_scenegraph");
  camera->config.set_output_window_name("main_window");
  camera->config.set_enable_stereo(true);
  camera->config.set_eye_dist(0.064);

  camera->get_pipeline_description()->get_resolve_pass()->tone_mapping_exposure(1.0f);

  auto left_screen = graph.add_node<gua::node::ScreenNode>("/nav/cam", "left_screen");
  left_screen->data.set_size(gua::math::vec2(0.08, 0.1));
  left_screen->translate(-0.04, 0, -0.05f);

  auto right_screen = graph.add_node<gua::node::ScreenNode>("/nav/cam", "right_screen");
  right_screen->data.set_size(gua::math::vec2(0.08, 0.1));
  right_screen->translate(0.04, 0, -0.05f);

  auto window = std::make_shared<gua::OculusSDK2Window>(":0.0", hmd);
  gua::WindowDatabase::instance()->add("main_window", window);
  window->config.set_enable_vsync(false);

  window->open();

  gua::Renderer renderer;

  gua::Timer timer;
  timer.start();

  double time(0);
  float desired_frame_time(1.0 / 60.0);
  gua::events::MainLoop loop;

  // application loop
  gua::events::Ticker ticker(loop, desired_frame_time);

  ticker.on_tick.connect([&]() {
    double frame_time(timer.get_elapsed());
    time += frame_time;
    timer.reset();

    std::function<void (std::shared_ptr<gua::node::Node>, int)> rotate;
    rotate = [&](std::shared_ptr<gua::node::Node> node, int depth) {
      node->rotate(frame_time * (1+depth) * 0.5, 1, 1, 0);
      for (auto child: node->get_children()) {
        rotate(child, ++depth);
      }
    };

    rotate(graph["/root_ape"], 1);

    for (int i = 0; i < lights.size(); ++i) {
      lights[i]->rotate(
          std::sin(time * (i * 0.1 + 0.5)) * frame_time * 2.5, 0, 1, 0);
    }

    graph["/root_ape"]->rotate(15 * frame_time, 0, 1, 0);

    //if (oculus_sensor) {
      camera->set_transform(get_oculus_transform(hmd));
    //}

    renderer.queue_draw({&graph});
  });

  loop.start();

  return 0;
}

