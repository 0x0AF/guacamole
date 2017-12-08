#ifndef PAGODA_LOADER_H
#define PAGODA_LOADER_H

#include "common.h"

class Loader
{
  public:
    Loader() = default;

    enum Flags
    {
        DEFAULTS = 0,
        LOAD_MATERIALS = 1 << 0,
        OPTIMIZE_GEOMETRY = 1 << 1,
        MAKE_PICKABLE = 1 << 2,
        NORMALIZE_POSITION = 1 << 3,
        NORMALIZE_SCALE = 1 << 4,
        NO_SHARED_MATERIALS = 1 << 5,
        OPTIMIZE_MATERIALS = 1 << 6
    };

    std::unordered_map<std::string, std::shared_ptr<::gua::node::Node>> loaded_files_ = std::unordered_map<std::string, std::shared_ptr<::gua::node::Node>>();

    std::shared_ptr<gua::node::Node> load_geometry(std::string const &file_name, unsigned flags)
    {
        std::shared_ptr<gua::node::Node> cached_node;
        std::string key(file_name + "_" + std::to_string(flags));
        auto searched(loaded_files_.find(key));

        if(searched != loaded_files_.end())
        {
            cached_node = searched->second;
        }
        else
        {
            bool fileload_succeed = false;

            if(is_supported(file_name))
            {
                cached_node = load(file_name, flags);
                cached_node->update_cache();

                loaded_files_.insert(std::make_pair(key, cached_node));

                // normalize mesh position and rotation
                if(flags & NORMALIZE_POSITION || flags & NORMALIZE_SCALE)
                {
                    auto bbox = cached_node->get_bounding_box();

                    if(flags & NORMALIZE_POSITION)
                    {
                        auto center((bbox.min + bbox.max) * 0.5f);
                        cached_node->translate(-center);
                    }

                    if(flags & NORMALIZE_SCALE)
                    {
                        auto size(bbox.max - bbox.min);
                        auto max_size(std::max(std::max(size.x, size.y), size.z));
                        cached_node->scale(1.f / max_size);
                    }
                }
                fileload_succeed = true;
            }

            if(!fileload_succeed)
            {
                gua::Logger::LOG_WARNING << "Unable to load " << file_name << ": Type is not supported!" << std::endl;
            }
        }

        return cached_node;
    }

    std::shared_ptr<gua::node::Node> create_geometry_from_file(std::string const &node_name, std::string const &file_name, std::shared_ptr<gua::Material> const &fallback_material, unsigned flags)
    {
        auto cached_node(load_geometry(file_name, flags));

        if(cached_node)
        {
            auto copy(cached_node->deep_copy());

            apply_fallback_material(copy, fallback_material, flags & NO_SHARED_MATERIALS);

            copy->set_name(node_name);
            return copy;
        }

        return std::make_shared<gua::node::TransformNode>(node_name);
    }

    std::shared_ptr<gua::node::Node> create_geometry_from_file(std::string const &node_name, std::string const &file_name, unsigned flags)
    {
        auto cached_node(load_geometry(file_name, flags));

        if(cached_node)
        {
            auto copy(cached_node->deep_copy());

            auto shader(gua::MaterialShaderDatabase::instance()->lookup("gua_default_material"));
            apply_fallback_material(copy, shader->make_new_material(), flags & NO_SHARED_MATERIALS);

            copy->set_name(node_name);
            return copy;
        }

        return std::make_shared<gua::node::TransformNode>(node_name);
    }

    std::shared_ptr<gua::node::Node> load(std::string const &file_name, unsigned flags)
    {
        gua::TextFile file(file_name);

        // MESSAGE("Loading mesh file %s", file_name.c_str());

        if(file.is_valid())
        {
            {
                auto importer = std::make_shared<Assimp::Importer>();

                unsigned ai_process_flags = aiProcessPreset_TargetRealtime_Quality | aiProcess_RemoveComponent;

                if(flags & OPTIMIZE_GEOMETRY)
                {
                    ai_process_flags |= aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_PreTransformVertices;
                }

                unsigned ai_ignore_flags = aiComponent_COLORS | aiComponent_ANIMATIONS | aiComponent_LIGHTS | aiComponent_CAMERAS | aiComponent_BONEWEIGHTS;

                if(!(flags & LOAD_MATERIALS))
                {
                    ai_ignore_flags |= aiComponent_MATERIALS;
                }

                importer->SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
                importer->SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, ai_ignore_flags);

                importer->ReadFile(file_name, ai_process_flags);

                aiScene const *scene(importer->GetScene());

                std::shared_ptr<gua::node::Node> new_node;

                std::string error = importer->GetErrorString();
                if(!error.empty())
                {
                    gua::Logger::LOG_WARNING << "load(): Importing failed, " << error << std::endl;
                }

                if(scene->mRootNode)
                {
                    unsigned count = 0;
                    new_node = get_tree(importer, scene, scene->mRootNode, file_name, flags, count);
                }
                else
                {
                    gua::Logger::LOG_WARNING << "Failed to load object \"" << file_name << "\": No valid root node contained!" << std::endl;
                }

                return new_node;
            }
        }

        gua::Logger::LOG_WARNING << "Failed to load object \"" << file_name << "\": File does not exist!" << std::endl;

        return nullptr;
    }

    std::vector<gua::TriMeshRessource *> const load_from_buffer(char const *buffer_name, unsigned buffer_size, bool build_kd_tree)
    {
        auto importer = std::make_shared<Assimp::Importer>();

        aiScene const *scene(importer->ReadFileFromMemory(buffer_name, buffer_size, aiProcessPreset_TargetRealtime_Quality | aiProcess_CalcTangentSpace));

        std::vector<gua::TriMeshRessource *> meshes;

        for(unsigned int n = 0; n < scene->mNumMeshes; ++n)
        {
            meshes.push_back(new gua::TriMeshRessource(gua::Mesh{*scene->mMeshes[n]}, build_kd_tree));
        }

        return meshes;
    }

    bool is_supported(std::string const &file_name) const
    {
        auto point_pos(file_name.find_last_of("."));
        Assimp::Importer importer;

        if(file_name.substr(point_pos + 1) == "raw")
        {
            return false;
        }
        return importer.IsExtensionSupported(file_name.substr(point_pos + 1));
    }

    std::shared_ptr<gua::node::Node> get_tree(std::shared_ptr<Assimp::Importer> const &importer, aiScene const *ai_scene, aiNode *ai_root, std::string const &file_name, unsigned flags,
                                              unsigned &mesh_count)
    {
        // creates a geometry node and returns it
        auto load_geometry = [&](int i) {
            gua::GeometryDescription desc("TriMesh", file_name, mesh_count++, flags);
            gua::GeometryDatabase::instance()->add(desc.unique_key(), std::make_shared<gua::TriMeshRessource>(gua::Mesh{*ai_scene->mMeshes[ai_root->mMeshes[i]]}, flags & MAKE_PICKABLE));

            // load material
            std::shared_ptr<gua::Material> material;
            unsigned material_index(ai_scene->mMeshes[ai_root->mMeshes[i]]->mMaterialIndex);

            if(flags & LOAD_MATERIALS)
            {
                gua::MaterialLoader material_loader;
                aiMaterial const *ai_material(ai_scene->mMaterials[material_index]);
                material = material_loader.load_material(ai_material, file_name, flags & OPTIMIZE_MATERIALS);
            }

            // return std::make_shared<node::TriMeshNode>("", desc.unique_key(),
            // material); // not allowed -> private c'tor
            return std::make_shared<gua::node::TriMeshNode>("", desc.unique_key(), material);
        };

        // there is only one child -- skip it!
        if(ai_root->mNumChildren == 1 && ai_root->mNumMeshes == 0)
        {
            return get_tree(importer, ai_scene, ai_root->mChildren[0], file_name, flags, mesh_count);
        }

        // there is only one geometry --- return it!
        if(ai_root->mNumChildren == 0 && ai_root->mNumMeshes == 1)
        {
            return load_geometry(0);
        }

        // else: there are multiple children and meshes
        auto group(std::make_shared<gua::node::TransformNode>());

        for(unsigned i(0); i < ai_root->mNumMeshes; ++i)
        {
            group->add_child(load_geometry(i));
        }

        for(unsigned i(0); i < ai_root->mNumChildren; ++i)
        {
            group->add_child(get_tree(importer, ai_scene, ai_root->mChildren[i], file_name, flags, mesh_count));
        }

        return group;
    }

    void apply_fallback_material(std::shared_ptr<gua::node::Node> const &root, std::shared_ptr<gua::Material> const &fallback_material, bool no_shared_materials)
    {
        auto g_node(std::dynamic_pointer_cast<gua::node::TriMeshNode>(root));

        if(g_node && !g_node->get_material())
        {
            g_node->set_material(fallback_material);
            g_node->update_cache();
        }
        else if(g_node && no_shared_materials)
        {
            g_node->set_material(std::make_shared<gua::Material>(*g_node->get_material()));
        }

        for(auto &child : root->get_children())
        {
            apply_fallback_material(child, fallback_material, no_shared_materials);
        }
    }
};

#endif // PAGODA_LOADER_H