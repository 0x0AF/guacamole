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

#ifndef GUA_ABUFFER_HPP
#define GUA_ABUFFER_HPP

// guacamole headers
#include <gua/platform.hpp>
#include <gua/math/math.hpp>
#include <gua/renderer/enums.hpp>
#include <gua/renderer/RenderContext.hpp>

namespace gua {

class GUA_DLL ABuffer {
 public:

  struct SharedResource {
    scm::gl::buffer_ptr  counter;
    scm::gl::buffer_ptr  frag_list;
    scm::gl::buffer_ptr  frag_data;
    size_t               frag_count = 0;
    scm::gl::texture_image_ptr max_depth;
    scm::gl::texture_image_ptr min_depth;
  };

  ABuffer() {}
  virtual ~ABuffer() {}

  void allocate(RenderContext& ctx, size_t buffer_size, math::vec2ui const& resolution);
  void allocate_shared(RenderContext const& with_ctx);
  void clear(RenderContext const& ctx, math::vec2ui const& resolution);
  void bind(RenderContext const& ctx);
  void unbind(RenderContext const& ctx);

 private:

  const size_t FRAG_LIST_WORD_SIZE = 8;
  const size_t FRAG_DATA_WORD_SIZE = 16;

  std::shared_ptr<SharedResource> res_ = nullptr;

};

}

#endif  // GUA_ABUFFER_HPP
