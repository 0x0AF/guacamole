---
layout: page
title : Features
header : Features
group: navigation
---
{% include JB/setup %}
{% include themes/hooligan/page_header.html %}

<div class="row">
  <div class="span4">
    <div class="well">
      huhu
    </div>
  </div>
  <div class="span4">
    <div class="well">
      huhu
    </div>
  </div>
  <div class="span4">
    <div class="well">
      huhu
    </div>
  </div>
</div>

<div class="row">
  <div class="span4">
    <div class="well">
      huhu
    </div>
  </div>
  <div class="span4">
    <div class="well">
      huhu
    </div>
  </div>
  <div class="span4">
    <div class="well">
      huhu
    </div>
  </div>
</div>

{% highlight c++ %}
#include <gua/guacamole.hpp>
#include <thread>
#include <chrono>

int main(int argc, char** argv) {

  // initialize guacamole
  gua::init(argc, argv);

  gua::ShadingModelDatabase::load_shading_models_from("data/materials/");
  gua::MaterialDatabase::load_materials_from("data/materials/");

  return 0;
}

{% endhighlight %}