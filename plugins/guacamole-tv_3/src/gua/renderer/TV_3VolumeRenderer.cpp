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

// class header
#include <gua/renderer/TV_3VolumeRenderer.hpp>
#include <gua/renderer/TV_3VolumePass.hpp>

// guacamole headers
#include <gua/renderer/TV_3Resource.hpp>
#include <gua/renderer/Pipeline.hpp>
#include <gua/renderer/ShaderProgram.hpp>
#include <gua/renderer/GBuffer.hpp>
#include <gua/renderer/ResourceFactory.hpp>

#include <gua/node/TV_3Node.hpp>
#include <gua/platform.hpp>
#include <gua/guacamole.hpp>
#include <gua/renderer/View.hpp>

#include <gua/databases.hpp>
#include <gua/utils/Logger.hpp>

#include <gua/config.hpp>

#include <gua/renderer/Window.hpp>

#include <scm/gl_core/data_formats.h>
#include <scm/gl_core/data_types.h>
#include <scm/gl_core/texture_objects/texture_image.h>

#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>

#include <scm/gl_core/shader_objects.h>
#include <scm/gl_core/render_device.h>

// external headers
#include <sstream>
#include <fstream>
#include <regex>
#include <list>
#include <boost/assign/list_of.hpp>

namespace gua {

  //////////////////////////////////////////////////////////////////////////////
  TV_3VolumeRenderer::TV_3VolumeRenderer() : TV_3Renderer() {
  }


  ///////////////////////////////////////////////////////////////////////////////
  void TV_3VolumeRenderer::_create_fbo_resources(gua::RenderContext const& ctx,
                                           scm::math::vec2ui const& render_target_dims) {
    // initialize FBO lazy during runtime
    volume_raycasting_fbo_.reset();

    // attachments
    volume_raycasting_color_result_ = ctx.render_device
      ->create_texture_2d(render_target_dims,
                          scm::gl::FORMAT_RGBA_8,
                          1, 1, 1);

    volume_raycasting_depth_result_ = ctx.render_device
      ->create_texture_2d(render_target_dims,
                          scm::gl::FORMAT_D32F,
                          1, 1, 1);
  }

  void TV_3VolumeRenderer::_clear_fbo_attachments(gua::RenderContext const& ctx) {
    if (!volume_raycasting_fbo_) {
      volume_raycasting_fbo_ = ctx.render_device->create_frame_buffer();
      volume_raycasting_fbo_->clear_attachments();
      volume_raycasting_fbo_
        ->attach_color_buffer(0, volume_raycasting_color_result_);
      volume_raycasting_fbo_
        ->attach_depth_stencil_buffer(volume_raycasting_depth_result_);

    ctx.render_context
    ->set_frame_buffer( volume_raycasting_fbo_ );
    }

    ctx.render_context
      ->clear_color_buffer(volume_raycasting_fbo_,
      0,
      scm::math::vec4f(0.0f, 0.0f, 0.0f, 0.0f));


    ctx.render_context
      ->clear_depth_stencil_buffer(volume_raycasting_fbo_);

    ctx.render_context
      ->set_frame_buffer(volume_raycasting_fbo_);
  }

  /////////////////////////////////////////////////////////////////////////////////////////////
  void TV_3VolumeRenderer::_raycasting_pass(gua::Pipeline& pipe, std::vector<gua::node::Node*> const& sorted_nodes, PipelinePassDescription const& desc) {

    RenderContext const& ctx(pipe.get_context());
    auto& scene = *pipe.current_viewstate().scene;

    auto view_matrix       = scene.rendering_frustum.get_view(); 
    auto projection_matrix = scene.rendering_frustum.get_projection();

    auto eye_position = math::vec4(scene.rendering_frustum.get_camera_position(),1.0);


   TV_3VolumePassDescription const& vol_rend_desc = static_cast<TV_3VolumePassDescription const&>(desc);

   auto const volume_render_mode = vol_rend_desc.mode();

   for (auto const& object : sorted_nodes) {

      auto tv_3_volume_node(reinterpret_cast<node::TV_3Node*>(object));

      //skip nodes which do not fit our pass description
      if( ( ( node::TV_3Node::NodeRenderMode::VOL_MAX_INTENSITY == tv_3_volume_node->mode() ) 
            && (TV_3VolumePassDescription::VolumeRenderMode::MAX_INTENSITY == volume_render_mode) ) ||
          ( ( node::TV_3Node::NodeRenderMode::VOL_AVG_INTENSITY == tv_3_volume_node->mode() ) 
                      && (TV_3VolumePassDescription::VolumeRenderMode::AVG_INTENSITY == volume_render_mode) ) ||
          ( ( node::TV_3Node::NodeRenderMode::VOL_COMPOSITING == tv_3_volume_node->mode() ) 
                      && (TV_3VolumePassDescription::VolumeRenderMode::COMPOSITING == volume_render_mode) ) ||
          ( ( node::TV_3Node::NodeRenderMode::VOL_ISOSURFACE == tv_3_volume_node->mode() ) 
                      && (TV_3VolumePassDescription::VolumeRenderMode::ISOSURFACE == volume_render_mode) ) 

            ) {
        continue; 
    }

      auto model_matrix = tv_3_volume_node->get_world_transform();
      auto mvp_matrix = projection_matrix * view_matrix * model_matrix;
      //forward_cube_shader_program_->apply_uniform(ctx, "gua_model_view_projection_matrix", math::mat4f(mvp_matrix));

      auto inv_model_mat = scm::math::inverse(model_matrix);

      math::vec4 model_space_eye_pos = inv_model_mat * eye_position;

      //forward_cube_shader_program_->apply_uniform(ctx, "gua_model_matrix", math::mat4f(tv_3_volume_node->get_world_transform()) ) ;
      
      forward_cube_shader_program_->use(ctx);
      forward_cube_shader_program_->apply_uniform(ctx, "gua_model_view_projection_matrix", math::mat4f(mvp_matrix));
      forward_cube_shader_program_->apply_uniform(ctx, "ms_eye_pos", math::vec4f(model_space_eye_pos/model_space_eye_pos[3]));
      forward_cube_shader_program_->apply_uniform(ctx, "volume_texture", 0);

      ctx.render_context->bind_vertex_array(box_vertex_array_ );
      //ctx.render_context->bind_index_buffer(box_element_buffer_, scm::gl::PRIMITIVE_TRIANGLE_LIST, scm::gl::TYPE_UINT);
 

      ctx.render_context->set_rasterizer_state(frontface_culling_rasterizer_state_);


     // ctx.render_context->uniform_sampler3D("volume_texture", 0);
      tv_3_volume_node->get_geometry()->bind_volume_texture(ctx, trilin_sampler_state_);
      ctx.render_context->apply();

  
      ctx.render_context->draw_arrays(scm::gl::PRIMITIVE_TRIANGLE_LIST, 0, 36*6);

    }
}

  /////////////////////////////////////////////////////////////////////////////////////////////
  void TV_3VolumeRenderer::_postprocessing_pass(gua::Pipeline& pipe, PipelinePassDescription const& desc) {
    RenderContext const& ctx(pipe.get_context());

    auto& target = *pipe.current_viewstate().target;
    bool write_depth = true;
    target.bind(ctx, write_depth);
    target.set_viewport(ctx);

    if(compositing_shader_program_ != nullptr) {
      compositing_shader_program_->use(ctx);
    }
    ctx.render_context->bind_texture(volume_raycasting_color_result_, trilin_sampler_state_, 0);

    compositing_shader_program_->apply_uniform(ctx, "blit_texture", 0);

    auto const& glapi = ctx.render_context->opengl_api();
    ctx.render_context->set_rasterizer_state(no_backface_culling_rasterizer_state_);
    ctx.render_context->apply();
    fullscreen_quad_->draw(ctx.render_context);
  }

}
